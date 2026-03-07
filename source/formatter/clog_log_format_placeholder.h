/**
 * @author Polaris
 * @date  2025/9/19
 */
#ifndef CASCA_LOG_CLOG_LOG_FORMAT_PLACEHOLDER_H
#define CASCA_LOG_CLOG_LOG_FORMAT_PLACEHOLDER_H

#include "clog_interpolator.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * create log format default placeholder map
 * @return log format placeholder map
 */
clog_hashmap_t *clog_log_format_default_placeholder_map_create(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_LOG_FORMAT_PLACEHOLDER_H */
