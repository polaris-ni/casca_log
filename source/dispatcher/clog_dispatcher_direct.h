/**
 * @author Polaris
 * @date 2026/2/10
 */

#ifndef CASCA_LOG_CLOG_DISPATCHER_DIRECT_H
#define CASCA_LOG_CLOG_DISPATCHER_DIRECT_H

#include "clog_dispatcher.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * create direct dispatcher
 * @return #clog_dispatcher_t
 */
clog_dispatcher_t *clog_dispatcher_direct_create(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DISPATCHER_DIRECT_H */
