/**
 * @auther polaris
 * @date  2025/10/17
 */
#include "clog_dispatcher_direct.h"
#include "casca_log.h"
#include "clog_recorder_manager.h"

static clog_res_e clog_dispatcher_direct_dispatch(clog_dispatcher_t* self, const uint32_t* recorders, size_t num,
                                                  const clog_item_t* item)
{
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

const clog_dispatcher_t* clog_dispatcher_direct(void)
{
    static const clog_dispatcher_t dispatcher = {.id = CLOG_DISPATCHER_ID_DIRECT,
                                                 .open = clog_dispatcher_empty_open,
                                                 .dispatch = clog_dispatcher_direct_dispatch,
                                                 .close = clog_dispatcher_empty_close,
                                                 .extra = NULL};
    return &dispatcher;
}
