/**
 * @author Polaris
 * @date 2026/3/13
 */

#ifndef CASCA_LOG_CLOG_RECORDER_SYSLOG_H
#define CASCA_LOG_CLOG_RECORDER_SYSLOG_H

#include "clog_recorder.h"
#ifndef CLOG_PLATFORM_WINDOWS
#include <syslog.h>
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * create syslog recorder
 * @param ident ident
 * @param opt log opt
 * @param facility facility
 * @return clog_recorder_t
 */
clog_recorder_t *clog_recorder_syslog_create(const char *ident, int opt, int facility);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_RECORDER_SYSLOG_H */
