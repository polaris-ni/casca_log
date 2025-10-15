/**
 * @auther polaris
 * @date  2025/10/16
 */
#include "clog_recorder_manager.h"

#include "casca_log.h"
#include "clog_hashmap.h"

static clog_hashmap_t* g_recorders;

clog_hashmap_t* clog_recorder_get_all(void)
{
    if (g_recorders == NULL) {
        g_recorders =
            clog_hashmap_create(sizeof(uint32_t), sizeof(clog_recorder_t), NULL, NULL, NULL, NULL, NULL, NULL, 0);
        return g_recorders;
    }
    return g_recorders;
}

clog_res_e clog_recorder_register(const clog_recorder_t* recorder)
{
    CLOG_RET_IF_NULL_X(recorder, CLOG_INVALID_PARAM, "recoder to be registered is NULL");
    const uint32_t id = recorder->id;
    CLOG_RET_IF_X(id < CLOG_RECODER_ID_RESERVED, CLOG_INVALID_PARAM, "recorder id %u is reserved", id);
    CLOG_RET_IF_X(id == CLOG_RECORDER_ID_INVALID, CLOG_INVALID_PARAM, "recorder id 0 is invalid");
    CLOG_RET_IF_NULL_X(recorder->setup, CLOG_INVALID_PARAM, "recoder setup func is NULL");
    CLOG_RET_IF_NULL_X(recorder->open, CLOG_INVALID_PARAM, "recoder open func is NULL");
    CLOG_RET_IF_NULL_X(recorder->write, CLOG_INVALID_PARAM, "reader write func is NULL");
    CLOG_RET_IF_NULL_X(recorder->flush, CLOG_INVALID_PARAM, "recorder flush func is NULL");
    CLOG_RET_IF_NULL_X(recorder->close, CLOG_INVALID_PARAM, "recorder close func is NULL");
    CLOG_RET_IF_NULL_X(recorder->cleanup, CLOG_INVALID_PARAM, "recoder setup func is NULL");
    clog_hashmap_t* map = clog_recorder_get_all();
    CLOG_RET_IF_NULL(map, CLOG_INVALID_PARAM);
    const void* data = clog_hashmap_get(map, &recorder->id);
    CLOG_RET_IF_X(data != NULL, CLOG_ALREADY_EXISTED, "recorder has existed, id = %u", recorder->id);
    const clog_res_e ret = clog_hashmap_put(map, &recorder->id, recorder);
    CLOG_RET_IF_FAILED_X(ret, "clog_hashmap_put failed, ret = %d", ret);
    return CLOG_SUCCESS;
}
