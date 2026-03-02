/**
 * @author Polaris
 * @date 2026/2/10
 */

#include "clog_dispatcher_direct.h"
#include "casca_log.h"
#include "clog_atomic_queue.h"
#include "clog_dispatcher_async_thread.h"
#include "clog_recorder_manager.h"
#include "clog_secure_func.h"

static clog_res_e clog_dispatcher_direct_open(clog_dispatcher_t *self, const clog_config_group_t *group)
{
    CLOG_UNUSED_VAR(self);
    CLOG_UNUSED_VAR(group);
    return CLOG_SUCCESS;
}

static void clog_dispatcher_direct_notify(clog_dispatcher_t *self, clog_dispatcher_event_e event)
{
    CLOG_RET_VOID_IF(event != CLOG_DISPATCHER_EVENT_DATA);
    clog_channel_t *channel = self->channel;
    const clog_item_t *item = NULL;
    clog_res_e res = channel->read(channel, &item);
    while (res == CLOG_SUCCESS && item != NULL) {
        for (size_t i = 0; i < CLOG_ARRAY_SIZE(item->recorder); ++i) {
            if (item->recorder[i] == CLOG_RECORDER_ID_INVALID) {
                break;
            }
            CLOG_IGNORE_RES(clog_recoder_write(item->recorder[i], item));
        }
        clog_release_log_item((clog_item_t *)item);
        item = NULL;
        res = channel->read(channel, &item);
    }
    if (item != NULL) {
        clog_release_log_item((clog_item_t *)item);
        item = NULL;
    }
}

static void clog_dispatcher_direct_close(clog_dispatcher_t *self)
{
    CLOG_UNUSED_VAR(self);
}

void clog_dispatcher_direct(clog_dispatcher_t *dispatcher)
{
    dispatcher->id = CLOG_DISPATCHER_ID_DIRECT;
    dispatcher->open = clog_dispatcher_direct_open;
    dispatcher->notify = clog_dispatcher_direct_notify;
    dispatcher->close = clog_dispatcher_direct_close;
    dispatcher->channel = NULL;
    dispatcher->extra = NULL;
}
