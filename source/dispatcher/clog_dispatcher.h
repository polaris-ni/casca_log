/**
 * @author polaris
 * @date  2025/10/17
 */
#ifndef CASCA_LOG_CLOG_DISPATCHER_H
#define CASCA_LOG_CLOG_DISPATCHER_H

#include "casca_log_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_DISPATCHER_ID_INVALID 0U
#define CLOG_DISPATCHER_ID_DIRECT 1U
#define CLOG_DISPATCHER_ID_RESERVED 1000U

typedef struct clog_dispatcher clog_dispatcher_t;

/**
 * open dispatcher, extra param should be initialized at this function
 * @param self clog_dispatcher_t itself
 * @return clog_res_e
 */
typedef clog_res_e (*clog_dispatcher_open_f)(clog_dispatcher_t* self);

/**
 * dispatcher log item to target recorders
 * @param self clog_dispatcher_t itself
 * @param recorders target recorders
 * @param num the num of target recorders
 * @param item log
 * @return clog_res_e
 */
typedef clog_res_e (*clog_dispatcher_dispatch_f)(clog_dispatcher_t* self, const uint32_t* recorders, size_t num,
                                                 const clog_item_t* item);

/**
 * close dispatcher, resources should be cleanup at this function
 * @param self clog_dispatcher_t itself
 * @return clog_res_e
 */
typedef void (*clog_dispatcher_close_f)(clog_dispatcher_t* self);

struct clog_dispatcher {
    uint32_t id; /* dispatcher id, 0 ~ 1000 is reserved */
    clog_dispatcher_open_f open;
    clog_dispatcher_dispatch_f dispatch;
    clog_dispatcher_close_f close;
    void* extra;
};

static clog_res_e clog_dispatcher_empty_open(clog_dispatcher_t* self)
{
    return CLOG_SUCCESS;
}

static clog_res_e clog_dispatcher_empty_dispatch(clog_dispatcher_t* self, const uint32_t* recorders, size_t num,
                                                 const clog_item_t* item)
{
    return CLOG_SUCCESS;
}

static void clog_dispatcher_empty_close(clog_dispatcher_t* self) {}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DISPATCHER_H */
