/**
 * @author polaris
 * @date  2025/10/17
 */
#ifndef CASCA_LOG_CLOG_DISPATCHER_H
#define CASCA_LOG_CLOG_DISPATCHER_H

#include "clog_channel_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_DISPATCHER_ID_INVALID 0U
#define CLOG_DISPATCHER_ID_DIRECT 1U
#define CLOG_DISPATCHER_ID_ASYNC_THREAD 2U
#define CLOG_DISPATCHER_ID_RESERVED 0xFFFFU

typedef struct clog_dispatcher clog_dispatcher_t;

typedef enum clog_dispatcher_event {
    CLOG_DISPATCHER_EVENT_START,
    CLOG_DISPATCHER_EVENT_DATA,
    CLOG_DISPATCHER_EVENT_END,
} clog_dispatcher_event_e;

/**
 * open dispatcher, extra param should be initialized at this function
 * @param self clog_dispatcher_t itself
 * @param group the parsed #clog_config_group_t from the Dispatchers.ClogXxx
 * @param channel current used channel
 * @return clog_res_e
 */
typedef clog_res_e (*clog_dispatcher_open_f)(clog_dispatcher_t* self, const clog_config_group_t* group,
                                             clog_channel_t* channel);

/**
 * notify dispatcher that a new log item has been put into channel
 * @param self clog_dispatcher_t itself
 * @param event event to be notified
 */
typedef void (*clog_dispatcher_notify_f)(clog_dispatcher_t* self, clog_dispatcher_event_e event);

/**
 * close dispatcher, resources should be cleanup at this function
 * @param self clog_dispatcher_t itself
 * @return clog_res_e
 */
typedef void (*clog_dispatcher_close_f)(clog_dispatcher_t* self);

struct clog_dispatcher {
    uint32_t id;
    clog_channel_t* channel;
    clog_dispatcher_open_f open;
    clog_dispatcher_notify_f notify;
    clog_dispatcher_close_f close;
    void* extra;
};

typedef void (*clog_dispatcher_provider_f)(clog_dispatcher_t* dispatcher);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DISPATCHER_H */
