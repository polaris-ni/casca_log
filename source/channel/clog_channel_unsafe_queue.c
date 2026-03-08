/**
 * @author Polaris
 * @date 2026/3/8
 */

#include "clog_channel_unsafe_queue.h"

#include "casca_log_core.h"
#include "clog_error.h"
#include "clog_queue.h"

static clog_res_e clog_unsafe_queue_channel_open(clog_channel_t *channel)
{
    clog_queue_t *queue = NULL;
    const clog_res_e ret = clog_queue_create(&queue, NULL);
    CLOG_RET_IF_FUNC_FAILED_X(clog_queue_create, ret);
    channel->param = queue;
    return CLOG_SUCCESS;
}

static clog_res_e clog_unsafe_queue_channel_write(clog_channel_t *channel, const clog_item_t *item)
{
    return clog_queue_enqueue(channel->param, (uintptr_t)item);
}

static clog_res_e clog_unsafe_queue_channel_read(clog_channel_t *channel, const clog_item_t **item)
{
    uintptr_t tmp = 0;
    const clog_res_e res = clog_queue_dequeue(channel->param, &tmp);
    if (res == CLOG_SUCCESS) {
        *item = (clog_item_t *)tmp;
    }
    return res;
}

static void clog_unsafe_queue_channel_close(clog_channel_t *channel)
{
    clog_queue_t *queue = channel->param;
    if (queue != NULL) {
        clog_res_e res = CLOG_SUCCESS;
        while (res == CLOG_SUCCESS) {
            uintptr_t tmp = 0;
            res = clog_queue_dequeue(channel->param, &tmp);
            if (res == CLOG_SUCCESS) {
                clog_release_log_item(channel->context, (clog_item_t *)tmp);
            }
        }
        clog_queue_destroy(&queue);
    }
    channel->param = NULL;
}

clog_channel_t *clog_unsafe_queue_channel_create()
{
    clog_channel_t *channel = clog_malloc(sizeof(clog_channel_t));
    CLOG_RET_IF_NULL_X(channel, NULL, "malloc clog_channel_t failed");
    channel->context = NULL;
    channel->id = CLOG_CHANNEL_ID_UNSAFE_QUEUE;
    channel->open = clog_unsafe_queue_channel_open;
    channel->write = clog_unsafe_queue_channel_write;
    channel->read = clog_unsafe_queue_channel_read;
    channel->close = clog_unsafe_queue_channel_close;
    channel->param = NULL;
    return channel;
}
