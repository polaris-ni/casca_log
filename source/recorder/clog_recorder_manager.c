/**
 * @auther polaris
 * @date  2025/10/16
 */
#include "clog_recorder_manager.h"
#include "casca_log.h"
#include "casca_log_keywords.h"
#include "clog_error.h"
#include "clog_hashmap.h"
#include "clog_recorder_file.h"
#include "clog_recorder_stdout.h"

static clog_hashmap_t *g_recorders = NULL;

static clog_recorder_provider_f g_customized_provider = NULL;

clog_res_e clog_recoder_write(uint32_t id, const clog_item_t *item) {
    CLOG_RET_IF_NULL(item, CLOG_INVALID_PARAM);
    clog_recorder_t *recorder = clog_hashmap_get(g_recorders, &id);
    CLOG_RET_IF_NULL(recorder, CLOG_TARGET_NOT_FOUND);
    clog_res_e ret = recorder->write(recorder, item);
    if (ret == CLOG_SUCCESS) {
        return CLOG_SUCCESS;
    }
    if (ret != CLOG_REQUEST_FLUSH) {
        CLOG_ERR_ADD("recorder %u write log item failed, ret = %u", id, ret);
        return ret;
    }
    ret = recorder->flush(recorder);
    if (ret != CLOG_SUCCESS) {
        CLOG_ERR_ADD("recorder %u flush failed, ret = %u", id, ret);
    }
    return CLOG_SUCCESS;
}

void clog_recorder_register_provider(clog_recorder_provider_f provider) {
    g_customized_provider = provider;
}

static void *clog_recoder_dup(const void *ptr) {
    clog_recorder_t *tmp = clog_malloc(sizeof(clog_recorder_t));
    CLOG_RET_IF_NULL(tmp, NULL);
    const clog_recorder_t *src = ptr;
    tmp->id = src->id;
    tmp->open = src->open;
    tmp->write = src->write;
    tmp->flush = src->flush;
    tmp->close = src->close;
    tmp->extra = src->extra;
    return tmp;
}

static void clog_recoder_free(void *ptr) {
    clog_recorder_t *tmp = ptr;
    tmp->close(tmp);
    clog_free(tmp);
}

static clog_res_e clog_recorder_get_origin_by_id(const char *name, uint32_t id, clog_recorder_t *recorder) {
    CLOG_RET_IF_X(id == CLOG_RECORDER_ID_INVALID, CLOG_INVALID_PARAM, "the id of recoder %s is invalid", name);
    if (id < CLOG_RECORDER_ID_RESERVED) {
        uint32_t ids[] = {CLOG_RECORDER_ID_STDOUT, CLOG_RECORDER_ID_FILE};
        const clog_recorder_t *recorders[] = {clog_recorder_stdout(), clog_recorder_file()};
        const size_t num = CLOG_ARRAY_SIZE(ids);
        CLOG_ASSERT(CLOG_ARRAY_SIZE(recorders) == num);
        CLOG_RET_IF_X(id > num, CLOG_OVERSIZE, "recorder not found, name is %s, id is %u", name, id);
        recorder->id = recorders[id - 1]->id;
        recorder->open = recorders[id - 1]->open;
        recorder->write = recorders[id - 1]->write;
        recorder->flush = recorders[id - 1]->flush;
        recorder->close = recorders[id - 1]->close;
        recorder->extra = recorders[id - 1]->extra;
        return CLOG_SUCCESS;
    }

    CLOG_RET_IF_NULL_X(g_customized_provider, CLOG_TARGET_NOT_FOUND, "customized recorder provider not set");
    const clog_res_e ret = g_customized_provider(id, recorder);
    CLOG_RET_IF_FAILED_X(ret, "get customized recorder failed, name is %s, id is %u, ret = %u", name, id, ret);
    return ret;
}

clog_res_e clog_recorder_setup(void) {
    g_recorders = clog_hashmap_create(sizeof(uint32_t), 0, NULL, NULL,
                                      clog_recoder_dup, clog_recoder_free, NULL, NULL, 0);
    CLOG_RET_IF_NULL_X(g_recorders, CLOG_NO_MEMORY, "clog_hashmap_create failed");
    const char *groups[] = {CLOG_STR_RECORDERS};
    const clog_config_group_t *group = clog_config_find_group(clog_get_config_root(), groups, CLOG_ARRAY_SIZE(groups));
    CLOG_RET_IF_NULL_X(group, CLOG_INVALID_PARAM, CLOG_STR_RECORDERS " not found");
    const clog_config_group_t *item = group->child;
    while (item != NULL) {
        uint32_t id = CLOG_RECORDER_ID_INVALID;
        clog_res_e ret = clog_config_find_item_in_group_uint(item, CLOG_STR_ID, &id);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_cleanup(), "get id of %s failed, ret = %d", item->name, ret);
        bool enabled = false;
        ret = clog_config_find_item_in_group_bool(item, CLOG_STR_ENABLED, &enabled);
        enabled = enabled || ret == CLOG_TARGET_NOT_FOUND;
        if (!enabled) {
            CLOG_CLEAN_RET_IF_X(ret != CLOG_TARGET_NOT_FOUND, clog_recorder_cleanup(), ret,
                                "get \"enabled\" of %s failed, ret = %d ", item->name, ret);
            continue;
        }
        clog_recorder_t tmp = {0};
        ret = clog_recorder_get_origin_by_id(item->name, id, &tmp);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_cleanup(), "get recorder %s failed, ret = %u", item->name, ret);
        ret = clog_hashmap_put(g_recorders, &id, &tmp);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_cleanup(), "add recorder %s failed, ret = %u", item->name, ret);
        clog_recorder_t *recorder = clog_hashmap_get(g_recorders, &id);
        CLOG_CLEAN_RET_IF_NULL_X(recorder, clog_recorder_cleanup(), CLOG_FAIL, "get recoder %s failed", item->name);
        ret = recorder->open(recorder, item);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_cleanup(), "open recorder %s failed, ret = %u", item->name, ret);
        item = item->sibling;
    }
    return CLOG_SUCCESS;
}

void clog_recorder_cleanup(void) {
    g_customized_provider = NULL;
    clog_hashmap_destroy(&g_recorders);
}
