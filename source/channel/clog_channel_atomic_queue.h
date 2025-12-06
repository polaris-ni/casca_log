/**
 * @author Polaris
 * @date  2025/12/4
 */
#ifndef CASCA_LOG_CLOG_CHANNEL_ATOMIC_QUEUE_H
#define CASCA_LOG_CLOG_CHANNEL_ATOMIC_QUEUE_H

#include "clog_channel_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * get atomic queue channel
 * @param channel channel
 * @return #clog_res_e
 */
void clog_atomic_queue_channel_provider(clog_channel_t* channel);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CHANNEL_ATOMIC_QUEUE_H */
