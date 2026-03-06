/**
 * @author Polaris
 * @date 2026/2/23
 */

#include "clog_recorder_file.h"
#include "casca_log_keywords.h"
#include "clog_error.h"
#include "clog_file_system.h"
#include "clog_interpolator.h"
#include "clog_interpolator_default_placeholder.h"
#include "clog_recorder_manager.h"

#define CLOG_RECORDER_FILE_META_INFO_VERSION 1U

typedef enum clog_recorder_file_split_type {
    CLOG_RECORDER_FILE_SPLIT_NONE, /* no spilt */
    CLOG_RECORDER_FILE_SPLIT_SIZE, /* spilt by size, value(unit: Byte) is the max size of a log file */
    CLOG_RECORDER_FILE_SPLIT_TIME, /* spilt by time, value(unit: Second) is the time interval between two log files */
    CLOG_RECORDER_FILE_SPLIT_NUMBER, /* spilt by num of log items, value(unit: Number) is the max num of log items */
    CLOG_RECORDER_FILE_SPLIT_MAX,
} clog_recorder_file_split_type_e;

CLOG_PACKED_STRUCT(clog_recorder_file_meta_info, {
    uint32_t version; /* meta metainfo version */
    uint32_t index; /* current file index */
    char file[256]; /* current process file */
    uint64_t timestamp; /* last file start time */
})

typedef struct clog_recorder_file_param {
    clog_interpolator_t *dir_interpolator;
    clog_interpolator_t *file_interpolator;
    clog_file_t *log;
    char metainfo_path[CASCA_LOG_FILEPATH_MAX_SIZE];
    clog_recorder_file_meta_info_t metainfo;
    clog_recorder_file_split_type_e split_type;
    union {
        struct {
            size_t max_file_size;
            size_t current_file_size;
        };
        struct {
            uint64_t max_interval;
            uint64_t start_time;
        };
        struct {
            uint32_t max_item_num;
            uint32_t current_item_num;
        };
    };
} clog_recorder_file_param_t;

static int clog_recorder_file_placeholder_log_index(void *param, char *buf, size_t size)
{
    CLOG_RET_IF_NULL_X(param, -CLOG_INVALID_PARAM, "param is null");
    const int ret = snprintf(buf, size, "%04u", ((clog_recorder_file_param_t *)param)->metainfo.index);
    CLOG_RET_IF_X(ret <= 0, -CLOG_FAIL, "snprintf failed, ret = %d", ret);
    return ret;
}

static clog_res_e clog_recorder_file_init_interpolator(const char *what, const clog_config_group_t *group,
                                                       const clog_interpolator_context_t *context,
                                                       clog_interpolator_t **interpolator)
{
    const char *value = NULL;
    const clog_res_e ret = clog_config_find_item_in_group_string(group, what, &value);
    CLOG_RET_IF_FAILED_X(ret, "find property %s failed, ret = %u", what, ret);
    return clog_interpolator_parse(context, value, interpolator);
}

static clog_res_e clog_recoder_file_open_file(clog_recorder_file_param_t *param)
{
    const char *file = param->metainfo.file;
    char directory[CASCA_LOG_FILEPATH_MAX_SIZE] = {0};
    clog_res_e ret = clog_file_get_dir(file, directory, sizeof(directory));
    CLOG_RET_IF_FAILED_X(ret, "clog_file_get_dir %s failed, ret = %u", file, ret);
    ret = clog_dir_create(directory, 0770);
    CLOG_RET_IF_X(ret != CLOG_SUCCESS && ret != CLOG_ALREADY_EXISTED, ret, "clog_dir_create %s failed, ret = %u",
                  directory, ret);
    param->log = clog_file_open(
        file, CLOG_FILE_READ | CLOG_FILE_WRITE | CLOG_FILE_CREATE | CLOG_FILE_EXIST | CLOG_FILE_SHARED, 0660);
    CLOG_RET_IF_NULL_X(param->log, CLOG_FAIL, "open log file %s failed", file);
    switch (param->split_type) {
        case CLOG_RECORDER_FILE_SPLIT_NONE:
            CLOG_IGNORE_RES(clog_file_seek(param->log, CLOG_FILE_SEEK_END, 0, NULL));
            return CLOG_SUCCESS;
        case CLOG_RECORDER_FILE_SPLIT_SIZE:
            CLOG_IGNORE_RES(clog_file_seek(param->log, CLOG_FILE_SEEK_SET, 0, NULL));
            size_t size = 0;
            CLOG_IGNORE_RES(clog_file_seek(param->log, CLOG_FILE_SEEK_END, 0, &size));
            param->current_file_size = size;
            return CLOG_SUCCESS;
        case CLOG_RECORDER_FILE_SPLIT_TIME:
            param->start_time = param->metainfo.timestamp;
            CLOG_IGNORE_RES(clog_file_seek(param->log, CLOG_FILE_SEEK_END, 0, NULL));
            return CLOG_SUCCESS;
        case CLOG_RECORDER_FILE_SPLIT_NUMBER:
            CLOG_IGNORE_RES(clog_file_seek(param->log, CLOG_FILE_SEEK_SET, 0, NULL));
            param->current_item_num = 0;
            while (true) {
                uint8_t data = 0;
                size_t num = 0;
                ret = clog_file_read(param->log, &data, sizeof(data), &num);
                if (ret != CLOG_SUCCESS || num != sizeof(data)) {
                    break;
                }
                if (data == '\n') {
                    param->current_item_num++;
                    CLOG_ERR_ADD("detect new line");
                }
            }
            CLOG_IGNORE_RES(clog_file_seek(param->log, CLOG_FILE_SEEK_END, 0, NULL));
            return CLOG_SUCCESS;
        default:
            CLOG_ERR_ADD("split type %u is invalid, should never go here", param->split_type);
            return CLOG_INVALID_PARAM;
    }
}

static clog_res_e clog_file_recorder_get_log_filepath(clog_recorder_file_param_t *param, char *filepath, size_t size)
{
    char path[CASCA_LOG_FILEPATH_MAX_SIZE] = {0};
    size_t num = 0;
    clog_res_e res = clog_interpolator_interpolate(param->dir_interpolator, NULL, path, sizeof(path), &num);
    CLOG_RET_IF_FAILED_X(res, "clog_interpolator_interpolate directory failed, ret = %u", res);
    res = clog_interpolator_interpolate(param->file_interpolator, param, path + num, sizeof(path) - num, NULL);
    CLOG_RET_IF_FAILED_X(res, "clog_interpolator_interpolate file failed, ret = %u", res);
    res = clog_normalize(path, filepath, size);
    CLOG_RET_IF_FAILED_X(res, "clog_normalize %s failed, ret = %u", path, res);
    return CLOG_SUCCESS;
}

static clog_res_e clog_recorder_file_init_meta_info(const clog_config_group_t *group,
                                                    const clog_interpolator_context_t *context,
                                                    clog_recorder_file_param_t *param)
{
    clog_interpolator_t *interpolator = NULL;
    clog_res_e res = clog_recorder_file_init_interpolator(CLOG_STR_METAINFO, group, context, &interpolator);
    CLOG_RET_IF_FAILED_X(res, "clog_recorder_file_init_interpolator failed, ret = %u", res);
    char tmp[CASCA_LOG_FILEPATH_MAX_SIZE] = {0};
    res = clog_interpolator_interpolate(interpolator, NULL, tmp, sizeof(tmp), NULL);
    CLOG_RET_IF_FAILED_X(res, "clog_recorder_file_init_interpolator failed, ret = %u", res);
    clog_interpolator_clear(&interpolator);
    res = clog_normalize(tmp, param->metainfo_path, sizeof(param->metainfo_path));
    CLOG_RET_IF_FAILED_X(res, "clog_normalize %s failed, ret = %u", tmp, res);
    clog_file_t *meta = clog_file_open(param->metainfo_path, CLOG_FILE_READ, 0660);
    if (meta == NULL) {
        param->metainfo.version = CLOG_RECORDER_FILE_META_INFO_VERSION;
        param->metainfo.index = 0;
        res = clog_file_recorder_get_log_filepath(param, param->metainfo.file, sizeof(param->metainfo.file));
        CLOG_RET_IF_FAILED_X(res, "clog_file_recorder_get_log_filepath %s failed, ret = %u", res);
        param->metainfo.index++;
        char directory[CASCA_LOG_FILEPATH_MAX_SIZE] = {0};
        clog_res_e ret = clog_file_get_dir(param->metainfo_path, directory, sizeof(directory));
        CLOG_RET_IF_FAILED_X(ret, "clog_file_get_dir %s failed, ret = %u", param->metainfo_path, ret);
        ret = clog_dir_create(directory, 0770);
        CLOG_RET_IF_X(ret != CLOG_SUCCESS && ret != CLOG_ALREADY_EXISTED, ret, "clog_dir_create %s failed, ret = %u",
                      directory, ret);
        meta = clog_file_open(param->metainfo_path, CLOG_FILE_WRITE | CLOG_FILE_CREATE | CLOG_FILE_TRUNCATE, 0660);
        CLOG_RET_IF_NULL_X(meta, CLOG_FAIL, "open meta info file %s failed", param->metainfo_path);
        res = clog_file_write(meta, &param->metainfo, sizeof(param->metainfo));
        clog_file_close(&meta);
        CLOG_RET_IF_FAILED_X(res, "write meta info file %s failed, ret = %u", param->metainfo_path, res);
    } else {
        res = clog_file_read(meta, &param->metainfo, sizeof(param->metainfo), NULL);
        clog_file_close(&meta);
        CLOG_RET_IF_FAILED_X(res, "read meta info file %s failed, ret = %u", param->metainfo_path, res);
    }
    return CLOG_SUCCESS;
}

static bool clog_recorder_file_need_flash(clog_recorder_file_param_t *param)
{
    switch (param->split_type) {
        case CLOG_RECORDER_FILE_SPLIT_NONE:
            return false;
        case CLOG_RECORDER_FILE_SPLIT_SIZE:
            return param->current_file_size >= param->max_file_size;
        case CLOG_RECORDER_FILE_SPLIT_TIME:
            return clog_timestamp_ms() - param->start_time >= param->max_interval;
        case CLOG_RECORDER_FILE_SPLIT_NUMBER:
            return param->current_item_num >= param->max_item_num;
        default:
            CLOG_ERR_ADD("split type %u is invalid", param->split_type);
            return false;
    }
}

static clog_res_e clog_recorder_file_init_param(const clog_config_group_t *group, clog_recorder_file_param_t *param)
{
    clog_hashmap_t *default_placeholders = clog_interpolator_default_placeholders();
    CLOG_RET_IF_NULL_X(default_placeholders, CLOG_FAIL, "clog_interpolator_default_placeholders failed");
    clog_interpolator_context_t *context = clog_interpolator_context_create(default_placeholders);
    CLOG_RET_IF_NULL_X(context, CLOG_FAIL, "clog_interpolator_context_create failed");
    clog_res_e res =
        clog_interpolator_context_register(context, "_log_index", clog_recorder_file_placeholder_log_index);
    if (res != CLOG_SUCCESS) {
        CLOG_ERR_ADD("register log index placeholder failed, ret = %u", res);
        goto RESULT_HANDLER;
    }
    res = clog_recorder_file_init_interpolator(CLOG_STR_DIRECTORY, group, context, &param->dir_interpolator);
    if (res != CLOG_SUCCESS) {
        CLOG_ERR_ADD("clog_recorder_file_init_interpolator directory failed, ret = %u", res);
        goto RESULT_HANDLER;
    }
    res = clog_recorder_file_init_interpolator(CLOG_STR_FILE, group, context, &param->file_interpolator);
    if (res != CLOG_SUCCESS) {
        CLOG_ERR_ADD("clog_recorder_file_init_interpolator file failed, ret = %u", res);
        goto RESULT_HANDLER;
    }
    res = clog_recorder_file_init_meta_info(group, context, param);
    if (res != CLOG_SUCCESS) {
        CLOG_ERR_ADD("clog_recorder_file_init_meta_info failed, ret = %u", res);
        goto RESULT_HANDLER;
    }
    res = clog_recoder_file_open_file(param);
    if (res != CLOG_SUCCESS) {
        CLOG_ERR_ADD("clog_recoder_file_open_file failed, ret = %u", res);
        goto RESULT_HANDLER;
    }
    res = CLOG_SUCCESS;

RESULT_HANDLER:
    clog_interpolator_context_destroy(&context);
    clog_hashmap_destroy(&default_placeholders);
    return res;
}

static clog_res_e clog_recorder_file_init_split(const clog_config_group_t *group, clog_recorder_file_param_t *param)
{
    const clog_config_item_t *arr = NULL;
    const clog_res_e ret = clog_config_find_item_in_group_array(group, CLOG_STR_SPLIT, &arr);
    CLOG_RET_IF_FAILED_X(ret, "find property " CLOG_STR_SPLIT "failed, ret = %u", ret);
    CLOG_RET_IF_X(arr->type != CLOG_CONFIG_TYPE_UINT, CLOG_INVALID_PARAM, "first param type(%u) of split is not uint",
                  arr->type);
    param->split_type = arr->value.uint;
    CLOG_RET_IF_X(param->split_type >= CLOG_RECORDER_FILE_SPLIT_MAX, CLOG_SUCCESS, "split type %u is invalid",
                  param->split_type);
    CLOG_RET_IF(param->split_type == CLOG_RECORDER_FILE_SPLIT_NONE, CLOG_SUCCESS);
    arr = arr->next;
    CLOG_RET_IF_NULL_X(arr, CLOG_INVALID_PARAM, "second param of split is absent");
    CLOG_RET_IF_X(arr->type != CLOG_CONFIG_TYPE_UINT, CLOG_INVALID_PARAM, "second param type(%u) of split is not uint",
                  arr->type);
    const uint32_t value = arr->value.uint;
    switch (param->split_type) {
        case CLOG_RECORDER_FILE_SPLIT_NONE:
            return CLOG_SUCCESS;
        case CLOG_RECORDER_FILE_SPLIT_SIZE:
            param->max_file_size = value;
            return CLOG_SUCCESS;
        case CLOG_RECORDER_FILE_SPLIT_TIME:
            param->max_interval = value * 1000U;
            return CLOG_SUCCESS;
        case CLOG_RECORDER_FILE_SPLIT_NUMBER:
            param->max_item_num = value;
            return CLOG_SUCCESS;
        default:
            CLOG_ERR_ADD("split type %u is invalid", param->split_type);
            return CLOG_INVALID_PARAM;
    }
}

static void clog_recorder_file_clear_param(clog_recorder_file_param_t *param)
{
    clog_interpolator_clear(&param->dir_interpolator);
    clog_interpolator_clear(&param->file_interpolator);
    clog_file_close(&param->log);
    clog_free(param);
}

static clog_res_e clog_recorder_file_open(clog_recorder_t *self, const clog_config_group_t *group)
{
    clog_recorder_file_param_t *param = clog_malloc(sizeof(clog_recorder_file_param_t));
    CLOG_RET_IF_NULL_X(param, CLOG_NO_MEMORY, "malloc clog_recorder_file_param_t failed");
    CLOG_IGNORE_RES(clog_memset(param, sizeof(clog_recorder_file_param_t), 0, sizeof(clog_recorder_file_param_t)));
    clog_res_e ret = clog_recorder_file_init_split(group, param);
    CLOG_RET_IF_FAILED_X(ret, "find property " CLOG_STR_SPLIT "failed, ret = %u", ret);
    ret = clog_recorder_file_init_param(group, param);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_file_clear_param(param), "init file recorder param failed, ret = %u",
                               ret);
    self->extra = param;
    if (clog_recorder_file_need_flash(param)) {
        ret = self->flush(self);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_file_clear_param(param),
                                   "clog file recorder flush on open failed, ret = %u", ret);
    }
    return CLOG_SUCCESS;
}

static clog_res_e clog_recorder_check_flush(clog_recorder_file_param_t *param, const clog_item_t *log)
{
    switch (param->split_type) {
        case CLOG_RECORDER_FILE_SPLIT_NONE:
            return CLOG_SUCCESS;
        case CLOG_RECORDER_FILE_SPLIT_SIZE:
            param->current_file_size += log->length;
            return param->current_file_size >= param->max_file_size ? CLOG_REQUEST_FLUSH : CLOG_SUCCESS;
        case CLOG_RECORDER_FILE_SPLIT_TIME:
            return log->timestamp - param->start_time >= param->max_interval ? CLOG_REQUEST_FLUSH : CLOG_SUCCESS;
        case CLOG_RECORDER_FILE_SPLIT_NUMBER:
            param->current_item_num += 1;
            return param->current_item_num >= param->max_item_num ? CLOG_REQUEST_FLUSH : CLOG_SUCCESS;
        default:
            CLOG_ERR_ADD("split type %u is invalid", param->split_type);
            return CLOG_INVALID_PARAM;
    }
}

static clog_res_e clog_recorder_file_write(clog_recorder_t *self, const clog_item_t *log)
{
    clog_recorder_file_param_t *param = self->extra;
    const clog_res_e res = clog_file_write(param->log, log->content, log->length);
    CLOG_RET_IF_FAILED_X(res, "write log to file failed, ret = %u", res);
    return clog_recorder_check_flush(param, log);
}

static clog_res_e clog_recorder_file_flush(clog_recorder_t *self)
{
    clog_recorder_file_param_t *param = self->extra;
    clog_file_t *old_log = param->log;
    /* 1. get new log file name */
    clog_res_e ret = clog_file_recorder_get_log_filepath(param, param->metainfo.file, sizeof(param->metainfo.file));
    CLOG_RET_IF_FAILED_X(ret, "get log filepath failed, ret = %u", ret);
    /* 2. update metainfo file */
    param->metainfo.timestamp = clog_timestamp_ms();
    param->metainfo.index++;
    clog_file_t *meta =
        clog_file_open(param->metainfo_path, CLOG_FILE_WRITE | CLOG_FILE_CREATE | CLOG_FILE_TRUNCATE, 0660);
    CLOG_RET_IF_NULL_X(param->metainfo_path, CLOG_FAIL, "open meta info file %s failed", param->metainfo_path);
    ret = clog_file_write(meta, &param->metainfo, sizeof(param->metainfo));
    clog_file_close(&meta);
    CLOG_RET_IF_FAILED_X(ret, "write meta info file %s failed, ret = %u", param->metainfo_path, ret);
    /* 3. open new log file */
    ret = clog_recoder_file_open_file(param);
    CLOG_RET_IF_FAILED_X(ret, "open log file %s failed, ret = %u", ret, param->metainfo.file);
    /* 4. close last file */
    clog_file_close(&old_log);
    return CLOG_SUCCESS;
}

static void clog_recorder_file_close(clog_recorder_t *self)
{
    CLOG_RET_VOID_IF_NULL(self->extra);
    clog_recorder_file_clear_param(self->extra);
    self->extra = NULL;
}

const clog_recorder_t *clog_recorder_file(void)
{
    static const clog_recorder_t recorder = {
        .id = CLOG_RECORDER_ID_FILE,
        .open = clog_recorder_file_open,
        .write = clog_recorder_file_write,
        .flush = clog_recorder_file_flush,
        .close = clog_recorder_file_close,
    };
    return &recorder;
}
