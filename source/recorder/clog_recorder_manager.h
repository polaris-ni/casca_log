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

/**
 * write log item to recoder
 * @param id the id of recorder
 * @param item log item
 * @return clog_res_e
 */
clog_res_e clog_recoder_write(uint32_t id, const clog_item_t* item);

/**
 * register customized recorders
 * @param recorders customized recorders
 * @param num the num of customized recorders
 * @return clog_res_e
 */
clog_res_e clog_recorder_register(const clog_recorder_t* recorders, size_t num);

/**
 * setup recorders
 * @return clog_res_e
 */
clog_res_e clog_recorder_setup(void);

/**
 * clean registered customized recorders and enabled recorders
 */
void clog_recorder_cleanup(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_RECORDER_MANAGER_H */
