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

clog_res_e clog_recorder_register(const clog_recorder_t* recorder);

clog_res_e clog_recorder_setup(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_RECORDER_MANAGER_H */
