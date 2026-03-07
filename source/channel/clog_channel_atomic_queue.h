/**
 * @author Polaris
 * @date  2025/12/4
 */
#ifndef CASCA_LOG_CLOG_CHANNEL_ATOMIC_QUEUE_H
#define CASCA_LOG_CLOG_CHANNEL_ATOMIC_QUEUE_H

#include "clog_channel.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * create atomic queue channel
 * @return channel
 */
clog_channel_t *clog_atomic_queue_channel_create();

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CHANNEL_ATOMIC_QUEUE_H */
