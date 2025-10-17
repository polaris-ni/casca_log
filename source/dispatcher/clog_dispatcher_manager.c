/**
 * @auther polaris
 * @date  2025/10/15
 */
#include "clog_dispatcher_manager.h"
#include "clog_error.h"
#include "clog_recorder_manager.h"

clog_res_e clog_dispatch(const uint32_t* recorders, size_t num, const clog_item_t* item)
{
    CLOG_RET_IF_NULL_X(recorders, CLOG_INVALID_PARAM, "target recorders is NULL");
    CLOG_RET_IF_NULL_X(item, CLOG_INVALID_PARAM, "item is NULL");
    clog_res_e last = CLOG_SUCCESS;
    for (size_t i = 0; i < num; ++i) {
        const clog_res_e ret = clog_recoder_write(recorders[i], item);
        if (ret != CLOG_SUCCESS) {
            clog_err_append_line("clog_recoder_write log %u to %u failed, ret = %d", item->seq, recorders[i], ret);
            last = ret;
        }
    }
    return last;
}
