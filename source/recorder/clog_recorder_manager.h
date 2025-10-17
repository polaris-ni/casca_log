/**
 * @author polaris
 * @date  2025/10/16
 */
#ifndef CASCA_LOG_CLOG_RECORDER_MANAGER_H
#define CASCA_LOG_CLOG_RECORDER_MANAGER_H

#include "clog_recorder.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_RECORDER_STDOUT_ID (1)

clog_res_e clog_recorder_register(const clog_recorder_t* recorders, size_t num);

clog_res_e clog_recorder_setup(void);

void clog_recorder_cleanup(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_RECORDER_MANAGER_H */
