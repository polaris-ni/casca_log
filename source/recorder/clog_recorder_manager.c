/**
 * @auther polaris
 * @date  2025/10/16
 */
#include "clog_recorder_manager.h"

#include "casca_log.h"
#include "casca_log_keywords.h"
#include "clog_error.h"
#include "clog_hashmap.h"
#include "clog_recorder_stdout.h"

static clog_hashmap_t* g_recorders = NULL;

static clog_recorder_t* g_customized_recorders = NULL;
static size_t g_customized_recorders_num = 0;

clog_res_e clog_recoder_write(uint32_t id, const clog_item_t* item)
{
    CLOG_RET_IF_NULL(item, CLOG_INVALID_PARAM);
    clog_recorder_t* recorder = clog_hashmap_get(g_recorders, &id);
    CLOG_RET_IF_NULL(recorder, CLOG_TARGET_NOT_FOUND);
    clog_res_e ret = recorder->write(recorder, item);
    if (ret == CLOG_SUCCESS) {
        return CLOG_SUCCESS;
    }
    if (ret != CLOG_REQUEST_FLUSH) {
        return ret;
    }
    ret = recorder->flush(recorder);
    if (ret != CLOG_SUCCESS) {
        recorder->close(recorder);
        return recorder->open(recorder);
    }
    return CLOG_SUCCESS;
}

clog_res_e clog_recorder_register(const clog_recorder_t* recorders, size_t num)
{
    CLOG_RET_IF_NULL_X(recorders, CLOG_INVALID_PARAM, "recoders to be registered is NULL");
    CLOG_RET_IF_X(num == 0, CLOG_INVALID_PARAM, "recoders num is 0");
    for (size_t i = 0; i < num; ++i) {
        const uint32_t id = recorders[i].id;
        CLOG_RET_IF_X(id <= CLOG_RECORDER_ID_RESERVED, CLOG_INVALID_PARAM,
                      "customized recorders %zu id (%u) is invalid", i, id);
        CLOG_RET_IF_NULL_X(recorders[i].setup, CLOG_INVALID_PARAM, "customized recorder %zu setup func is NULL", i);
        CLOG_RET_IF_NULL_X(recorders[i].open, CLOG_INVALID_PARAM, "customized recorder %zu open func is NULL", i);
        CLOG_RET_IF_NULL_X(recorders[i].write, CLOG_INVALID_PARAM, "customized recorder %zu write func is NULL", i);
        CLOG_RET_IF_NULL_X(recorders[i].flush, CLOG_INVALID_PARAM, "customized recorder %zu flush func is NULL", i);
        CLOG_RET_IF_NULL_X(recorders[i].close, CLOG_INVALID_PARAM, "customized recorder %zu close func is NULL", i);
        CLOG_RET_IF_NULL_X(recorders[i].cleanup, CLOG_INVALID_PARAM, "customized recorder %zu cleanup func is NULL", i);
    }
    clog_recorder_t* tmp = clog_malloc(sizeof(clog_recorder_t) * num);
    CLOG_RET_IF_NULL_X(tmp, CLOG_INVALID_PARAM, "clog_malloc failed");
    for (size_t i = 0; i < num; ++i) {
        CLOG_IGNORE_RES(clog_memcpy(&tmp[i], sizeof(clog_recorder_t), &recorders[i], sizeof(clog_recorder_t)));
        tmp[i].extra = NULL;
    }
    g_customized_recorders = tmp;
    g_customized_recorders_num = num;
    return CLOG_SUCCESS;
}

static void* clog_recoder_dup(const void* ptr)
{
    clog_recorder_t* tmp = (clog_recorder_t*)clog_malloc(sizeof(clog_recorder_t));
    CLOG_RET_IF_NULL(tmp, NULL);
    const clog_recorder_t* src = (const clog_recorder_t*)ptr;
    tmp->id = src->id;
    tmp->setup = src->setup;
    tmp->open = src->open;
    tmp->write = src->write;
    tmp->flush = src->flush;
    tmp->close = src->close;
    tmp->cleanup = src->cleanup;
    tmp->extra = NULL;
    return tmp;
}

static void clog_recoder_free(void* ptr)
{
    clog_recorder_t* tmp = (clog_recorder_t*)ptr;
    tmp->close(tmp);
    tmp->cleanup(NULL);
    clog_free(tmp);
}

static const clog_recorder_t* clog_recorder_get_origin_by_id(const char* name, uint32_t id)
{
    CLOG_RET_IF_X(id == CLOG_RECORDER_ID_INVALID, NULL, "the id of recoder %s is invalid", name);
    if (id < CLOG_RECORDER_ID_RESERVED) {
        uint32_t ids[] = {CLOG_RECORDER_STDOUT_ID};
        const clog_recorder_t* recorders[] = {clog_recorder_stdout()};
        const size_t num = CLOG_ARRAY_SIZE(ids);
        CLOG_ASSERT(CLOG_ARRAY_SIZE(recorders) == num);
        CLOG_RET_IF_X(id > num, NULL, "recorder not found, name is %s, id is %u", name, id);
        return recorders[id - 1];
    }

    CLOG_RET_IF_X((g_customized_recorders == NULL) || (g_customized_recorders_num == 0), NULL,
                  "customized recorder not found, name is %s, id is %u", name, id);
    const clog_recorder_t* recorder = NULL;
    for (size_t i = 0; i < g_customized_recorders_num; ++i) {
        if (g_customized_recorders[i].id == id) {
            recorder = &g_customized_recorders[i];
            break;
        }
    }

    CLOG_RET_IF_NULL_X(recorder, NULL, "customized recorder not found, name is %s, id is %u", name, id);
    return recorder;
}

clog_res_e clog_recorder_setup(void)
{
    g_recorders =
        clog_hashmap_create(sizeof(uint32_t), 0, NULL, NULL, clog_recoder_dup, clog_recoder_free, NULL, NULL, 0);
    CLOG_RET_IF_NULL_X(g_recorders, CLOG_NO_MEMORY, "clog_hashmap_create failed");
    const char* groups[] = {CLOG_STR_RECORDERS};
    const clog_config_group_t* group = clog_config_find_group(clog_get_config_root(), groups, CLOG_ARRAY_SIZE(groups));
    CLOG_RET_IF_NULL_X(group, CLOG_INVALID_PARAM, CLOG_STR_RECORDERS " not found");
    const clog_config_group_t* item = group->child;
    while (item != NULL) {
        uint32_t id = CLOG_RECORDER_ID_INVALID;
        clog_res_e ret = clog_config_find_item_in_group_uint(item, CLOG_STR_ID, &id);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_cleanup(), "get id of %s failed, ret = %d", item->name, ret);
        bool enabled = false;
        ret = clog_config_find_item_in_group_bool(item, CLOG_STR_ENABLED, &enabled);
        enabled = enabled || (ret == CLOG_TARGET_NOT_FOUND);
        if (!enabled) {
            CLOG_CLEAN_RET_IF_X(ret != CLOG_TARGET_NOT_FOUND, clog_recorder_cleanup(), ret, "enabled of %s not found",
                                item->name);
            continue;
        }
        const clog_recorder_t* recorder = clog_recorder_get_origin_by_id(item->name, id);
        CLOG_CLEAN_RET_IF_NULL(recorder, clog_recorder_cleanup(), CLOG_TARGET_NOT_FOUND);
        ret = clog_hashmap_put(g_recorders, &id, recorder);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_cleanup(), "add recorder %s failed, ret = %d", item->name, ret);
        clog_recorder_t* tmp = clog_hashmap_get(g_recorders, &id);
        CLOG_CLEAN_RET_IF_NULL_X(tmp, clog_recorder_cleanup(), CLOG_FAIL, "get recoder of %s by id failed", item->name);
        ret = tmp->setup(tmp, item);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_cleanup(), "setup recorder %s failed, ret = %d", item->name, ret);
        ret = tmp->open(tmp);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_cleanup(), "open recorder %s failed, ret = %d", item->name, ret);
        item = item->sibling;
    }
    if (g_customized_recorders != NULL) {
        clog_free(g_customized_recorders);
        g_customized_recorders = NULL;
    }
    g_customized_recorders_num = 0;
    return CLOG_SUCCESS;
}

void clog_recorder_cleanup(void)
{
    if (g_customized_recorders != NULL) {
        clog_free(g_customized_recorders);
        g_customized_recorders = NULL;
    }
    g_customized_recorders_num = 0;
    clog_hashmap_destroy(&g_recorders);
}
