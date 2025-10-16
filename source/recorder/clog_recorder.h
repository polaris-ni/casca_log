/**
 * @author polaris
 * @date  2025/10/15
 */
#ifndef CASCA_LOG_CLOG_RECORDER_H
#define CASCA_LOG_CLOG_RECORDER_H

#include "casca_log_base.h"
#include "clog_config.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_RECORDER_ID_INVALID 0u /* invalid recorder id */
#define CLOG_RECORDER_ID_RESERVED 1000u /* 0 ~ 1000 is reserved for internal recorder, > 1000 for customized id */

typedef struct clog_recorder clog_recorder_t;

/**
 * setup recorder, it will be called only once when #clog_recorder_setup
 * if there are extra params, it should be set to #extra of #clog_recorder_t
 * @param self itself
 * @param group the parsed #clog_config_group_t from the Recorders.ClogXxx
 * @return #clog_res_e
 */
typedef clog_res_e (*clog_recorder_setup_f)(clog_recorder_t* self, const clog_config_group_t* group);

/**
 * open recorder
 * @param self itself
 * @return #clog_res_e
 */
typedef clog_res_e (*clog_recorder_open_f)(clog_recorder_t* self);

/**
 * write log to recorder
 * @param self itself
 * @param log log item
 * @return #clog_res_e
 */
typedef clog_res_e (*clog_recorder_write_f)(clog_recorder_t* self, const clog_item_t* log);

/**
 * flush recorder
 * this func will be called when #clog_recorder_write_f return #CLOG_REQUEST_FLUSH or #clog_destroy is called
 * @param self itself
 * @return #clog_res_e
 */
typedef clog_res_e (*clog_recorder_flush_f)(clog_recorder_t* self);

/**
 * close recoder
 * @param self itself
 */
typedef void (*clog_recorder_close_f)(clog_recorder_t* self);

/**
 * cleanup recoder, it will be called when #clog_recorder_cleanup
 * remember to free #extra if it is set by #clog_recorder_setup_f
 * @param self itself
 */
typedef void (*clog_recorder_cleanup_f)(clog_recorder_t* self);

struct clog_recorder {
    uint32_t id;
    clog_recorder_setup_f setup;
    clog_recorder_open_f open;
    clog_recorder_write_f write;
    clog_recorder_flush_f flush;
    clog_recorder_close_f close;
    clog_recorder_cleanup_f cleanup;
    void* extra;
};

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_RECORDER_H */
