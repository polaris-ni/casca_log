/**
 * @auther Polaris
 * @date  2025/12/4
 */
#include "clog_channel_atomic_queue.h"
#include "casca_log.h"
#include "clog_atomic_mpsc_queue.h"
#include "clog_error.h"
#include "clog_hooks.h"

typedef struct clog_dispatcher_async_thread_param {
    clog_atomic_mpsc_queue_t *queue;
} clog_atomic_queue_channel_param_t;

static clog_res_e clog_atomic_queue_channel_open(clog_channel_t *channel, const clog_config_group_t *config)
{
    CLOG_UNUSED_VAR(config);
    clog_atomic_queue_channel_param_t *param = clog_malloc(sizeof(clog_atomic_queue_channel_param_t));
    CLOG_RET_IF_NULL_X(param, CLOG_NO_MEMORY, "malloc atomic queue channel param failed");
    param->queue = clog_atomic_mpsc_queue_create();
    CLOG_CLEAN_RET_IF_NULL_X(param->queue, clog_free(param), CLOG_NO_MEMORY, "create atomic queue failed");
    channel->param = param;
    return CLOG_SUCCESS;
}

static clog_res_e clog_atomic_queue_channel_write(clog_channel_t *channel, const clog_item_t *item)
{
    const clog_atomic_queue_channel_param_t *param = channel->param;
    return clog_atomic_mpsc_queue_in(param->queue, (uintptr_t)item);
}

static clog_res_e clog_atomic_queue_channel_read(clog_channel_t *channel, const clog_item_t **item)
{
    const clog_atomic_queue_channel_param_t *param = channel->param;
    uintptr_t tmp = 0;
    const clog_res_e res = clog_atomic_mpsc_queue_out(param->queue, &tmp);
    if (res == CLOG_SUCCESS) {
        *item = (clog_item_t *)tmp;
    }
    return res;
}

static void clog_atomic_queue_channel_close(clog_channel_t *channel)
{
    clog_atomic_queue_channel_param_t *param = channel->param;
    if (param != NULL) {
        clog_atomic_mpsc_queue_destroy(param->queue);
        clog_free(param);
    }
    channel->param = NULL;
}

void clog_atomic_queue_channel_provider(clog_channel_t *channel)
{
    CLOG_RET_VOID_IF_NULL_X(channel, "channel is null");
    channel->id = CLOG_CHANNEL_ID_ATOMIC_QUEUE;
    channel->open = clog_atomic_queue_channel_open;
    channel->write = clog_atomic_queue_channel_write;
    channel->read = clog_atomic_queue_channel_read;
    channel->close = clog_atomic_queue_channel_close;
    channel->param = NULL;
}

clog_channel_t *clog_atomic_queue_channel_create()
{
    clog_channel_t *channel = clog_malloc(sizeof(clog_channel_t));
    CLOG_RET_IF_NULL_X(channel, NULL, "malloc clog_channel_t failed");
    channel->id = CLOG_CHANNEL_ID_ATOMIC_QUEUE;
    channel->open = clog_atomic_queue_channel_open;
    channel->write = clog_atomic_queue_channel_write;
    channel->read = clog_atomic_queue_channel_read;
    channel->close = clog_atomic_queue_channel_close;
    channel->param = NULL;
    return channel;
}
