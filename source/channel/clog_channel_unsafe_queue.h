/**
 * @author Polaris
 * @date 2026/3/8
 */

#ifndef CASCA_LOG_CLOG_CHANNEL_UNSAFE_QUEUE_H
#define CASCA_LOG_CLOG_CHANNEL_UNSAFE_QUEUE_H

#include "clog_channel.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * create thread unsafe queue channel, used for one thread usage
 * @return channel
 */
clog_channel_t *clog_unsafe_queue_channel_create();

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CHANNEL_UNSAFE_QUEUE_H */
