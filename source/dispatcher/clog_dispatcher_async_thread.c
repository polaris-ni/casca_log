/**
 * @auther Polaris
 * @date  2025/11/27
 */
#include "clog_dispatcher_async_thread.h"
#ifndef errno
#include <errno.h>
#endif
#include "casca_log.h"
#include "clog_semaphore.h"
#include "clog_thread.h"
#include "clog_atomic_types.h"
#include "clog_error.h"
#include "clog_recorder_manager.h"
#include "clog_secure_func.h"

typedef struct clog_dispatcher_async_thread_param {
    clog_thread_t thread;
    atomic_uintptr_t state;
    clog_sem_t *sem;
} clog_dispatcher_async_thread_param_t;

typedef enum clog_async_thread_dispatcher_state {
    CLOG_ASYNC_THREAD_DISPATCHER_IDLE,
    CLOG_ASYNC_THREAD_DISPATCHER_RUNNING,
    CLOG_ASYNC_THREAD_DISPATCHER_STOPPING,
    CLOG_ASYNC_THREAD_DISPATCHER_CLOSED,
} clog_async_thread_dispatcher_state_e;

static void clog_async_thread_handler(void *args, size_t size) {
    CLOG_ASSERT(size == sizeof(clog_dispatcher_t));
    CLOG_ASSERT(args != NULL);
    const clog_dispatcher_t *dispatcher = args;
    CLOG_ASSERT(dispatcher->extra != NULL);
    clog_dispatcher_async_thread_param_t *param = dispatcher->extra;
    clog_channel_t *channel = dispatcher->channel;
    while (true) {
        const int ret = clog_sem_wait(param->sem, CLOG_SEM_TIMEOUT_NEVER);
        if (ret != 0) {
            CLOG_ERR_ADD("sem_wait failed, ret = %d, err = %d", ret, errno);
            continue;
        }
        const clog_async_thread_dispatcher_state_e state = clog_atomic_get(&param->state);
        if (state == CLOG_ASYNC_THREAD_DISPATCHER_STOPPING) {
            clog_atomic_set(&param->state, CLOG_ASYNC_THREAD_DISPATCHER_CLOSED);
            return;
        }
        if (state != CLOG_ASYNC_THREAD_DISPATCHER_RUNNING) {
            return;
        }
        const clog_item_t *item = NULL;
        clog_res_e res = channel->read(channel, &item);
        while (res == CLOG_SUCCESS && item != NULL) {
            for (size_t i = 0; i < CLOG_ARRAY_SIZE(item->recorder); ++i) {
                if (item->recorder[i] == CLOG_RECORDER_ID_INVALID) {
                    break;
                }
                CLOG_IGNORE_RES(clog_recoder_write(item->recorder[i], item));
            }
            clog_release_log_item((clog_item_t *) item);
            item = NULL;
            res = channel->read(channel, &item);
        }
        if (item != NULL) {
            clog_release_log_item((clog_item_t *) item);
            item = NULL;
        }
    }
}

static clog_res_e clog_dispatcher_async_thread_open(clog_dispatcher_t *self, const clog_config_group_t *group) {
    CLOG_UNUSED_VAR(group);
    clog_dispatcher_async_thread_param_t *param = clog_malloc(sizeof(clog_dispatcher_async_thread_param_t));
    CLOG_RET_IF_NULL_X(param, CLOG_NO_MEMORY, "malloc async thread param failed");
    param->sem = clog_sem_create(0);
    if (param->sem == NULL) {
        CLOG_ERR_ADD("clog_sem_create failed");
        clog_free(param);
        return CLOG_FAIL;
    }
    clog_atomic_set(&param->state, CLOG_ASYNC_THREAD_DISPATCHER_IDLE);
    self->extra = param;
    const clog_res_e res =
            clog_thread_create(&param->thread, NULL, clog_async_thread_handler, self, sizeof(clog_dispatcher_t));
    if (res != CLOG_SUCCESS) {
        CLOG_ERR_ADD("clog_thread_create failed, ret = %u", res);
        CLOG_IGNORE_RES(clog_sem_destroy(param->sem));
        param->sem = NULL;
        clog_free(param);
        self->extra = NULL;
    } else {
        clog_thread_detach(param->thread);
    }
    return res;
}

static void clog_dispatcher_async_thread_notify(clog_dispatcher_t *self, clog_dispatcher_event_e event) {
    clog_dispatcher_async_thread_param_t *param = self->extra;
    CLOG_ASSERT(param != NULL);
    switch (event) {
        case CLOG_DISPATCHER_EVENT_START:
            /* do nothing */
            clog_atomic_set(&param->state, CLOG_ASYNC_THREAD_DISPATCHER_RUNNING);
            CLOG_IGNORE_RES(clog_sem_post(param->sem));
            break;
        case CLOG_DISPATCHER_EVENT_DATA:
            CLOG_IGNORE_RES(clog_sem_post(param->sem));
            break;
        case CLOG_DISPATCHER_EVENT_END:
            clog_atomic_set(&param->state, CLOG_ASYNC_THREAD_DISPATCHER_STOPPING);
            CLOG_IGNORE_RES(clog_sem_post(param->sem));
            break;
        default:
            break;
    }
}

static void clog_dispatcher_async_thread_close(clog_dispatcher_t *self) {
    clog_dispatcher_async_thread_param_t *param = self->extra;
    const clog_async_thread_dispatcher_state_e state = clog_atomic_get(&param->state);
    if (state == CLOG_ASYNC_THREAD_DISPATCHER_RUNNING) {
        clog_atomic_set(&param->state, CLOG_ASYNC_THREAD_DISPATCHER_STOPPING);
        CLOG_IGNORE_RES(clog_sem_post(param->sem));
    }
    /* wait thread process over */
    while (clog_atomic_get(&param->state) != CLOG_ASYNC_THREAD_DISPATCHER_CLOSED) {
    }
    CLOG_IGNORE_RES(clog_sem_destroy(param->sem));
    CLOG_IGNORE_RES(clog_memset(&param->thread, sizeof(param->thread), 0, sizeof(param->thread)));
    clog_atomic_set(&param->state, CLOG_ASYNC_THREAD_DISPATCHER_IDLE);
    clog_free(param);
    self->extra = NULL;
}

void clog_dispatcher_async_thread(clog_dispatcher_t *dispatcher) {
    dispatcher->id = CLOG_DISPATCHER_ID_ASYNC_THREAD;
    dispatcher->open = clog_dispatcher_async_thread_open;
    dispatcher->notify = clog_dispatcher_async_thread_notify;
    dispatcher->close = clog_dispatcher_async_thread_close;
    dispatcher->channel = NULL;
    dispatcher->extra = NULL;
}
