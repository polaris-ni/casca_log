/**
 * @author polaris
 * @date  2025/10/15
 */
#ifndef CASCA_LOG_CLOG_DISPATCHER_H
#define CASCA_LOG_CLOG_DISPATCHER_H

#include "casca_log_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * dispatcher log item to #recorders
 * @param recoders recoder id array
 * @param num recoder num
 * @param item log item
 * @return #clog_res_e
 */
clog_res_e clog_dispatch(const uint32_t* recoders, size_t num, const clog_item_t* item);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DISPATCHER_H */
