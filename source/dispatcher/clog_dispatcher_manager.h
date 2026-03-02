/**
 * @author polaris
 * @date  2025/10/15
 */
#ifndef CASCA_LOG_CLOG_DISPATCHER_MANAGER_H
#define CASCA_LOG_CLOG_DISPATCHER_MANAGER_H

#include "clog_dispatcher.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * register customized dispatchers
 * @param dispatchers customized dispatchers
 * @param num the num of customized dispatchers
 * @return #clog_res_e
 */
clog_res_e clog_dispatcher_register(const clog_dispatcher_t *dispatchers, size_t num);

/**
 * setup dispatchers
 * @return #clog_res_e
 */
clog_res_e clog_dispatcher_setup(void);

/**
 * notify event to dispatcher
 * @param event clog_dispatcher_event_e
 */
void clog_dispatcher_notify(clog_dispatcher_event_e event);

/**
 * cleanup dispatchers
 * @return #clog_res_e
 */
void clog_dispatcher_cleanup(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_DISPATCHER_MANAGER_H */
