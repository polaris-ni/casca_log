/**
 * @author Polaris
 * @date  2025/9/3
 */
#include "casca_log.h"
#include "casca_log_keywords.h"
#include "clog_channel_base.h"
#include "clog_channel_manager.h"
#include "clog_config.h"
#include "clog_config_ext.h"
#include "clog_datetime.h"
#include "clog_dispatcher_manager.h"
#include "clog_error.h"
#include "clog_formatter.h"
#include "clog_hashmap.h"
#include "clog_platform.h"
#include "clog_recorder_manager.h"
#include "clog_secure_func.h"
#include "clog_thread.h"
#ifdef CLOG_PLATFORM_LINUX
#include <unistd.h>
#endif

typedef struct clog_context {
    const char *process;
    clog_hashmap_t *modules;
    clog_config_t config;

    struct {
        clog_interpolator_t *interpolator;

        struct {
            struct {
                const char *trace;
                const char *debug;
                const char *info;
                const char *warn;
                const char *error;
                const char *fetal;
            } tag;
        } level;
    } formatter;

    struct {
        clog_filter_t pre;
        clog_filter_t post;
    } filters;
#ifdef CASCA_LOG_MEM_POOL
    clog_buffer_pool_t *pool;
#endif
    clog_channel_t channel;
} clog_context_t;

static clog_context_t g_context = {0};

static const clog_setup_f g_setup_funcs[] = {
    clog_formatter_setup,
    clog_filter_setup,
    clog_dispatcher_setup,
    clog_recorder_setup,
};

static const clog_cleanup_f g_cleanup_funcs[] = {
    clog_formatter_cleanup,
    clog_filter_cleanup,
    clog_dispatcher_cleanup,
    clog_recorder_cleanup,
};

static clog_res_e clog_init_process_attrs(const char *process, const clog_config_group_t *group)
{
    uint32_t value = 0;
    const clog_res_e ret = clog_config_find_item_in_group_uint(group, "level", &value);
    CLOG_RET_IF_X(ret != CLOG_SUCCESS, ret, "level not found or invalid in process[%s], ret = %d", process, ret);
    if (value > CLOG_LEVEL_ALL) {
        CLOG_ERR_ADD("the value of level is invalid, value = %u", value);
        return CLOG_ERROR_FORMAT;
    }
    g_context.config.level = value;
    return CLOG_SUCCESS;
}

static clog_res_e clog_init_process_module(const char *process, const clog_config_group_t *module, clog_hashmap_t *map)
{
    bool enabled = true;
    clog_res_e ret = clog_config_item_get_enabled(module, &enabled);
    CLOG_RET_IF_FAILED(ret);
    CLOG_RET_IF(!enabled, CLOG_SUCCESS); /* module is not enabled, skip parse */
    uint32_t value = 0;
    ret = clog_config_find_item_in_group_uint(module, CLOG_STR_LEVEL, &value);
    if (ret == CLOG_TARGET_NOT_FOUND) {
        value = g_context.config.level;
    } else {
        CLOG_RET_IF_X(ret != CLOG_SUCCESS, ret, "level invalid in [%s.%s], ret = %d", process, module->name, ret);
    }
    if (value > CLOG_LEVEL_ALL) {
        CLOG_ERR_ADD("the value of level is invalid, value = %u", value);
        return CLOG_ERROR_FORMAT;
    }
    clog_module_t tmp = {.level = value, .num = 0, .recorders = {0}};
    const clog_config_item_t *recorders = clog_config_find_item_in_group(module, CLOG_STR_RECORDER);
    if (recorders != NULL) {
        CLOG_RET_IF_X(recorders->type != CLOG_CONFIG_TYPE_ARRAY, CLOG_ERROR_FORMAT,
                      "\"recoder\" of module %s should be array", module->name);
        clog_config_item_t *data = recorders->value.array;
        while (data != NULL) {
            CLOG_RET_IF_X(data->type != CLOG_CONFIG_TYPE_UINT, CLOG_ERROR_FORMAT,
                          "\"recoder\" of module %s should be unsigned int", module->name);
            CLOG_RET_IF_X(tmp.num > CLOG_ARRAY_SIZE(tmp.recorders), CLOG_OVERSIZE,
                          "recoder num of %s oversize, max num is %zu", module->name, CLOG_ARRAY_SIZE(tmp.recorders));
            tmp.recorders[tmp.num] = data->value.uint;
            tmp.num++;
            data = data->next;
        }
    }
    return clog_hashmap_put(map, module->name, &tmp);
}

static clog_res_e clog_init_process(const char *process, const clog_config_group_t *root, clog_hashmap_t **modules)
{
    clog_hashmap_t *map =
        clog_hashmap_create(0, sizeof(clog_module_t), clog_hashmap_string_dup, clog_hashmap_string_free, NULL, NULL,
                            clog_hashmap_string_cmp, clog_hashmap_string_size, 0);
    CLOG_RET_IF_NULL_X(map, CLOG_NO_MEMORY, "clog_hashmap_create failed");
    const char *groups[] = {"Process", process};
    const clog_config_group_t *group = clog_config_find_group(root, groups, CLOG_ARRAY_SIZE(groups));
    if (group == NULL) {
        clog_hashmap_destroy(&map);
        CLOG_ERR_ADD("process [%s] not found", process);
        return CLOG_TARGET_NOT_FOUND;
    }
    clog_res_e ret = clog_init_process_attrs(process, group);
    if (ret != CLOG_SUCCESS) {
        clog_hashmap_destroy(&map);
        return ret;
    }
    const clog_config_group_t *child = group->child;
    while (child != NULL) {
        ret = clog_init_process_module(process, child, map);
        if (ret != CLOG_SUCCESS) {
            clog_hashmap_destroy(&map);
            return ret;
        }
        child = child->sibling;
    }
    *modules = map;
    return CLOG_SUCCESS;
}

#ifdef CASCA_LOG_MEM_POOL
static clog_res_e clog_init_buffer_pool(clog_context_t *context, const clog_config_group_t *root)
{
    const char *groups[] = {CLOG_STR_PERFORMANCE, CLOG_STR_BUFFER_POOL};
    const clog_config_group_t *config = clog_config_find_group(root, groups, CLOG_ARRAY_SIZE(groups));
    CLOG_RET_IF_NULL_X(config, CLOG_TARGET_NOT_FOUND, "config Performance.BufferPool not found");
    bool auto_manage = true;
    clog_res_e ret = clog_config_find_item_in_group_bool(config, CLOG_STR_AUTO, &auto_manage);
    if (ret != CLOG_SUCCESS) {
        CLOG_RET_IF_X(ret != CLOG_TARGET_NOT_FOUND, ret,
                      "parse config \"auto\" in Performance.BufferPool failed, ret = %u", ret);
        auto_manage = true;
    }
    uint32_t capacity = 0;
    ret = clog_config_find_item_in_group_uint(config, CLOG_STR_CAPACITY, &capacity);
    CLOG_RET_IF_FAILED_X(ret, "parse config \"capacity\" in Performance.BufferPool failed, ret = %u", ret);
    uint32_t threshold = 0;
    ret = clog_config_find_item_in_group_uint(config, CLOG_STR_THRESHOLD, &threshold);
    CLOG_RET_IF_FAILED_X(ret, "parse config \"threshold\" in Performance.BufferPool failed, ret = %u", ret);
    CLOG_RET_IF_X(capacity == 0 || capacity >= 100, CLOG_INVALID_PARAM, "capacity(%u) is invalid", capacity);
    ret = clog_buffer_pool_initialize(&context->pool, sizeof(clog_item_t), capacity, auto_manage, threshold);
    CLOG_RET_IF_FAILED_X(ret, "clog_buffer_pool_initialize failed, ret = %u", ret);
    return CLOG_SUCCESS;
}
#endif

static clog_res_e clog_init_channel(clog_channel_t *channel, const clog_config_group_t *root)
{
    const char *groups[] = {CLOG_STR_CHANNELS};
    const clog_config_group_t *config = clog_config_find_group(root, groups, CLOG_ARRAY_SIZE(groups));
    CLOG_RET_IF_NULL_X(config, CLOG_TARGET_NOT_FOUND, "config " CLOG_STR_CHANNELS " not found");
    uint32_t use = CLOG_CHANNEL_ID_INVALID;
    clog_res_e ret = clog_config_find_item_in_group_uint(config, CLOG_STR_USE, &use);
    CLOG_RET_IF_FAILED_X(ret, "parse config " CLOG_STR_USE " in " CLOG_STR_CHANNELS " failed, ret = %u", ret);
    CLOG_RET_IF_X(use == CLOG_CHANNEL_ID_INVALID, CLOG_INVALID_PARAM, "the value of " CLOG_STR_USE " is invalid");
    const clog_config_group_t *child = config->child;
    while (child != NULL) {
        uint32_t id = CLOG_CHANNEL_ID_INVALID;
        ret = clog_config_find_item_in_group_uint(child, CLOG_STR_ID, &id);
        CLOG_RET_IF_FAILED_X(ret, "parse config " CLOG_STR_ID " in %s failed, ret = %u", child->name, ret);
        if (id == use) {
            bool enabled = true;
            ret = clog_config_find_item_in_group_bool(child, CLOG_STR_ENABLED, &enabled);
            CLOG_RET_IF_FAILED_X(ret, "parse config " CLOG_STR_ENABLED " in %s failed, ret = %u", child->name, ret);
            CLOG_RET_IF_X(!enabled, CLOG_ABNORMAL_STATE, "the channel in use(id = %u) is disabled", use);
            ret = clog_channel_get(use, channel);
            CLOG_RET_IF_FAILED_X(ret, "clog_channel_get failed, ret = %u", ret);
            ret = channel->open(channel, child);
            CLOG_CLEAN_RET_IF_FAILED_X(ret, channel->close(channel), "open %s failed, ret = %u", child->name, ret);
            return CLOG_SUCCESS;
        }
        child = child->sibling;
    }
    CLOG_ERR_ADD("the channel in use(id = %u) not found", use);
    return CLOG_TARGET_NOT_FOUND;
}

clog_res_e clog_init(const char *process, const char *config)
{
    CLOG_RET_IF_NULL_X(process, CLOG_INVALID_PARAM, "process is NULL");
    CLOG_RET_IF_NULL_X(config, CLOG_INVALID_PARAM, "config is NULL");
    g_context.process = clog_strdup(process);
    CLOG_RET_IF_NULL_X(g_context.process, CLOG_NO_MEMORY, "clog_strdup process failed");

    g_context.config.root = clog_config_parse(config);
    CLOG_CLEAN_RET_IF_NULL_X(g_context.config.root, clog_destroy(NULL, 0), CLOG_ERROR_FORMAT,
                             "clog_config_parse failed");

    clog_res_e ret = clog_init_process(process, g_context.config.root, &g_context.modules);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_destroy(NULL, 0), "clog_init_process failed, ret = %u", ret);

#ifdef CASCA_LOG_MEM_POOL
    ret = clog_init_buffer_pool(&g_context, g_context.config.root);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_destroy(NULL, 0), "clog_init_buffer_pool failed, ret = %u", ret);
#endif

    ret = clog_init_channel(&g_context.channel, g_context.config.root);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_destroy(NULL, 0), "clog_init_channel failed, ret = %u", ret);
    return CLOG_SUCCESS;
}

clog_res_e clog_setup(const clog_setup_f *funcs, const size_t num)
{
    CLOG_RET_IF_X(funcs == NULL && num != 0, CLOG_INVALID_PARAM, "funcs is NULL, but num is %zu", num);
    const size_t count = CLOG_ARRAY_SIZE(g_setup_funcs);
    clog_res_e ret;
    for (size_t i = 0; i < count; i++) {
        ret = g_setup_funcs[i]();
        CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    }
    CLOG_RET_IF(num == 0, CLOG_SUCCESS);
    for (size_t i = 0; i < num; ++i) {
        ret = funcs[i]();
        CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    }
    return CLOG_SUCCESS;
}

const clog_config_group_t *clog_get_config_root(void)
{
    return g_context.config.root;
}

const char *clog_get_process(void)
{
    return g_context.process == NULL ? "NULL" : g_context.process;
}

const char *clog_get_level_tag(const clog_level_e level)
{
    switch (level) {
        case CLOG_LEVEL_TRACE:
            return g_context.formatter.level.tag.trace;
        case CLOG_LEVEL_DEBUG:
            return g_context.formatter.level.tag.debug;
        case CLOG_LEVEL_INFO:
            return g_context.formatter.level.tag.info;
        case CLOG_LEVEL_WARN:
            return g_context.formatter.level.tag.warn;
        case CLOG_LEVEL_ERROR:
            return g_context.formatter.level.tag.error;
        case CLOG_LEVEL_FETAL:
            return g_context.formatter.level.tag.fetal;
        default:
            return "NULL";
    }
}

void clog_set_level_tag(const clog_level_e level, const char *tag)
{
    switch (level) {
        case CLOG_LEVEL_TRACE:
            g_context.formatter.level.tag.trace = tag;
            break;
        case CLOG_LEVEL_DEBUG:
            g_context.formatter.level.tag.debug = tag;
            break;
        case CLOG_LEVEL_INFO:
            g_context.formatter.level.tag.info = tag;
            break;
        case CLOG_LEVEL_WARN:
            g_context.formatter.level.tag.warn = tag;
            break;
        case CLOG_LEVEL_ERROR:
            g_context.formatter.level.tag.error = tag;
            break;
        case CLOG_LEVEL_FETAL:
            g_context.formatter.level.tag.fetal = tag;
            break;
        default:
            CLOG_ERR_ADD("level %u is invalid", level);
            break;
    }
}

clog_interpolator_t *clog_get_interpolator(void)
{
    return g_context.formatter.interpolator;
}

void clog_set_interpolator(clog_interpolator_t *interpolator)
{
    g_context.formatter.interpolator = interpolator;
}

const clog_filter_t *clog_get_filters(const clog_filter_type_e type)
{
    if (type == CLOG_FILTER_PRE) {
        return g_context.filters.pre.next;
    }
    if (type == CLOG_FILTER_POST) {
        return g_context.filters.post.next;
    }
    return NULL;
}

void clog_set_filters(const clog_filter_t *pre, const clog_filter_t *post)
{
    g_context.filters.pre.next = pre;
    g_context.filters.post.next = post;
}

const clog_module_t *clog_get_module_info(const char *module)
{
    return clog_hashmap_get(g_context.modules, module);
}

#ifdef CASCA_LOG_MEM_POOL
clog_buffer_pool_t *clog_get_buffer_pool(void)
{
    return g_context.pool;
}
#endif

clog_item_t *clog_acquire_log_item(void)
{
#ifdef CASCA_LOG_MEM_POOL
    CLOG_ASSERT(g_context.pool != NULL);
    return clog_buffer_pool_acquire(g_context.pool);
#else
    return clog_malloc(sizeof(clog_item_wrapper_t));
#endif
}

void clog_release_log_item(clog_item_t *item)
{
#ifdef CASCA_LOG_MEM_POOL
    CLOG_ASSERT(g_context.pool != NULL);
    clog_buffer_pool_release(g_context.pool, item);
#else
    clog_free(item);
#endif
}

clog_channel_t *clog_get_channel(void)
{
    return &g_context.channel;
}

void clog_destroy(const clog_cleanup_f *funcs, const size_t num)
{
    const size_t count = CLOG_ARRAY_SIZE(g_cleanup_funcs);
    for (size_t i = 0; i < count; i++) {
        g_cleanup_funcs[i]();
    }
    CLOG_SAFE_FREE(g_context.process);
    clog_filter_free((clog_filter_t *)g_context.filters.pre.next);
    g_context.filters.pre.next = NULL;
    clog_filter_free((clog_filter_t *)g_context.filters.post.next);
    g_context.filters.post.next = NULL;
    clog_config_destroy_group(g_context.config.root);
    clog_hashmap_destroy(&g_context.modules);
    if (g_context.channel.close != NULL) {
        g_context.channel.close(&g_context.channel);
        CLOG_IGNORE_RES(clog_memset(&g_context.channel, sizeof(clog_channel_t), 0, sizeof(clog_channel_t)));
    }
#ifdef CASCA_LOG_MEM_POOL
    if (g_context.pool != NULL) {
        clog_buffer_pool_finalize(g_context.pool);
    }
#endif
    g_context.config.root = NULL;
    g_context.config.level = CLOG_LEVEL_OFF;
    if (funcs != NULL && num != 0) {
        for (size_t i = 0; i < num; ++i) {
            funcs[i]();
        }
    }
}

static void clog_item_init_datetime(clog_item_t *item)
{
    clog_datetime_t time = {0};
    clog_datetime_now(&time);
    item->year = time.year;
    item->month = time.month;
    item->day = time.day;
    item->hour = time.hour;
    item->minute = time.minute;
    item->second = time.second;
    item->millisecond = time.millisecond;
    item->timestamp = clog_timestamp_of_datetime(&time, CLOG_TIMEZONE_OF(+, 8, 0));
}

static bool clog_module_check(const clog_module_t *info, clog_level_e level, uint32_t recorder)
{
    CLOG_RET_IF(((1 << (level - 1)) & info->level) == 0, false);
    for (size_t i = 0; i < info->num; ++i) {
        if (info->recorders[i] == recorder) {
            return true;
        }
    }
    return false;
}

static clog_res_e clog_log_internal(const uint32_t *recorders, size_t num, const clog_item_wrapper_t *wrapper)
{
    /* prefilter */
    bool pass = clog_filter_log(clog_get_filters(CLOG_FILTER_PRE), wrapper);
    CLOG_RET_IF_X(!pass, CLOG_NOT_PERMITTED, "clog_filter_log PRE failed");
    /* formatter */
    size_t length = 0;
    const clog_res_e ret = clog_format_log(g_context.formatter.interpolator, wrapper, wrapper->log->content,
                                           sizeof(wrapper->log->content), &length);
    CLOG_RET_IF_FAILED_X(ret, "clog_format_log failed, ret = %u", ret);
    wrapper->log->length = (uint16_t)length;
    /* postfilter */
    pass = clog_filter_log(clog_get_filters(CLOG_FILTER_POST), wrapper);
    CLOG_RET_IF_X(!pass, CLOG_NOT_PERMITTED, "clog_filter_log POST failed");
    for (size_t i = 0; i < num && i < CLOG_ARRAY_SIZE(wrapper->log->recorder); ++i) {
        wrapper->log->recorder[i] = recorders[i];
    }
    /* add to channel */
    const clog_res_e res = g_context.channel.write(&g_context.channel, wrapper->log);
    if (res == CLOG_SUCCESS) {
        /* notify dispatcher to handle log */
        clog_dispatcher_notify(CLOG_DISPATCHER_EVENT_DATA);
    }
    return res;
}

static clog_res_e clog_init_item_wrapper(const char *module, const char *file, const char *function, const int line,
                                         const clog_level_e level, const char *fmt, clog_item_wrapper_t *wrapper)
{
    clog_item_t *item = clog_acquire_log_item();
    CLOG_RET_IF_NULL_X(item, CLOG_NO_MEMORY, "clog_acquire_log_item failed");
    wrapper->process = g_context.process;
    wrapper->module = module;
    wrapper->filename = file;
    wrapper->function = function;
    wrapper->fmt = fmt;
    wrapper->log = item;
    clog_item_init_datetime(wrapper->log);
    wrapper->log->tid = clog_thread_self();
    wrapper->log->line = line;
    wrapper->log->level = level;
    return CLOG_SUCCESS;
}

clog_res_e clog_log(const char *module, uint32_t recorder, const char *file, const char *function, const int line,
                    const clog_level_e level, const char *fmt, ...)
{
    clog_err_clear();
    const clog_module_t *info = clog_get_module_info(module);
    CLOG_RET_IF_NULL_X(info, CLOG_TARGET_NOT_FOUND, "module info %s not found", module);
    CLOG_RET_IF(!clog_module_check(info, level, recorder), CLOG_NOT_PERMITTED);
    clog_item_wrapper_t wrapper;
    const clog_res_e res = clog_init_item_wrapper(module, file, function, line, level, fmt, &wrapper);
    CLOG_RET_IF_FAILED_X(res, "clog_init_item_wrapper failed, res = %u", res);
    va_start(wrapper.args, fmt);
    const clog_res_e ret = clog_log_internal(&recorder, 1, &wrapper);
    va_end(wrapper.args);
    return ret;
}

clog_res_e clog_module_log(const char *module, const char *file, const char *function, int line, clog_level_e level,
                           const char *fmt, ...)
{
    clog_err_clear();
    const clog_module_t *info = clog_get_module_info(module);
    CLOG_RET_IF_NULL_X(info, CLOG_TARGET_NOT_FOUND, "module info %s not found", module);
    CLOG_RET_IF(((1 << (level - 1)) & info->level) == 0, CLOG_NOT_PERMITTED);
    CLOG_RET_IF(info->num == 0, CLOG_TARGET_NOT_FOUND);
    clog_item_wrapper_t wrapper;
    const clog_res_e res = clog_init_item_wrapper(module, file, function, line, level, fmt, &wrapper);
    CLOG_RET_IF_FAILED_X(res, "clog_init_item_wrapper failed, res = %u", res);
    va_start(wrapper.args, fmt);
    const clog_res_e ret = clog_log_internal(info->recorders, info->num, &wrapper);
    va_end(wrapper.args);
    return ret;
}
