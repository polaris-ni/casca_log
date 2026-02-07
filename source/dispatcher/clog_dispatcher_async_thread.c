/**
 * @auther Polaris
 * @date  2025/11/27
 */
#include "clog_dispatcher_async_thread.h"

#include "casca_log.h"
#include "clog_atomic_queue.h"
#include "clog_error.h"

static clog_res_e clog_dispatcher_async_thread_open(clog_dispatcher_t* self, const clog_config_group_t* group)
{
    clog_dispatcher_async_thread_param_t* param = clog_malloc(sizeof(clog_dispatcher_async_thread_param_t));
    CLOG_RET_IF_NULL_X(param, CLOG_NO_MEMORY, "malloc async thread param failed");
    param->queue = clog_atomic_mpsc_queue_create();
    CLOG_CLEAN_RET_IF_NULL_X(param->queue, clog_free(param), CLOG_NO_MEMORY, "create async thread queue failed");
    param->pool = clog_get_buffer_pool();
    return CLOG_SUCCESS;
}

static clog_res_e clog_dispatcher_async_thread_dispatch(clog_dispatcher_t* self, const uint32_t* recorders, size_t num,
                                                        const clog_item_t* item)
{
    clog_dispatcher_async_thread_param_t* param = self->extra;
    CLOG_RET_IF_NULL(param, CLOG_INVALID_PARAM);
    return clog_atomic_mpsc_queue_in(param->queue, (uintptr_t)item);
}

static void clog_dispatcher_async_thread_close(clog_dispatcher_t* self)
{
    clog_dispatcher_async_thread_param_t* param = self->extra;
    if (param != NULL) {
        clog_atomic_mpsc_queue_destroy(param->queue);
        clog_free(param);
    }
    self->extra = NULL;
}

void clog_dispatcher_async_thread(clog_dispatcher_t* dispatcher)
{
    dispatcher->id = CLOG_DISPATCHER_ID_ASYNC_THREAD;
    dispatcher->open = clog_dispatcher_async_thread_open;
    dispatcher->close = clog_dispatcher_async_thread_close;
    dispatcher->extra = NULL;
}
