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
 * customized recorder provider function
 * @param id recorder id
 * @param recorder customized recorder
 * @return #clog_res_e
 */
typedef clog_res_e (*clog_recorder_provider_f)(uint32_t id, clog_recorder_t *recorder);

/**
 * open recorder
 * @param self itself
 * @param group the parsed #clog_config_group_t from the Recorders.ClogXxx
 * @return #clog_res_e
 */
typedef clog_res_e (*clog_recorder_open_f)(clog_recorder_t *self, const clog_config_group_t *group);

/**
 * write log to recorder
 * @param self itself
 * @param log log item
 * @return #clog_res_e
 */
typedef clog_res_e (*clog_recorder_write_f)(clog_recorder_t *self, const clog_item_t *log);

/**
 * flush recorder
 * this func will be called when #clog_recorder_write_f return #CLOG_REQUEST_FLUSH or #clog_destroy is called
 * @param self itself
 * @return #clog_res_e
 */
typedef clog_res_e (*clog_recorder_flush_f)(clog_recorder_t *self);

/**
 * close recoder
 * @param self itself
 */
typedef void (*clog_recorder_close_f)(clog_recorder_t *self);

struct clog_recorder {
    uint32_t id;
    clog_recorder_open_f open;
    clog_recorder_write_f write;
    clog_recorder_flush_f flush;
    clog_recorder_close_f close;
    void *extra;
};

static clog_res_e clog_recorder_empty_open(clog_recorder_t *self, const clog_config_group_t *group)
{
    CLOG_UNUSED_VAR(self);
    return CLOG_SUCCESS;
}

static clog_res_e clog_recorder_empty_write(clog_recorder_t *self, const clog_item_t *log)
{
    CLOG_UNUSED_VAR(self);
    CLOG_UNUSED_VAR(log);
    return CLOG_SUCCESS;
}

static clog_res_e clog_recorder_empty_flush(clog_recorder_t *self)
{
    CLOG_UNUSED_VAR(self);
    return CLOG_SUCCESS;
}

static void clog_recorder_empty_close(clog_recorder_t *self)
{
    CLOG_UNUSED_VAR(self);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_RECORDER_H */
