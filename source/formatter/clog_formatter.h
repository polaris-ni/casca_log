/**
 * @author Polaris
 * @date  2025/10/1
 */
#ifndef CASCA_LOG_CLOG_FORMATTER_H
#define CASCA_LOG_CLOG_FORMATTER_H

#include "casca_log_defines.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * formatter setup, init placeholders
 * @return clog_res_e
 */
clog_res_e clog_formatter_setup(void);

void clog_formatter_cleanup(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_FORMATTER_H */
