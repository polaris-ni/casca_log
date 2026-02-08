/**
 * @author Polaris
 * @date  2025/10/12
 */
#ifndef CASCA_LOG_CLOG_FILTER_H
#define CASCA_LOG_CLOG_FILTER_H

#include <stdbool.h>
#include "casca_log_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum clog_filter_type {
    CLOG_FILTER_PRE, /* prefilter, will be executed before formatter, item.content is NULL */
    CLOG_FILTER_POST /* postfilter, will be executed after formatter, item.content is valid */
} clog_filter_type_e;

typedef enum clog_filter_res {
    CLOG_FILTER_CONTINUE, /* pass to next filter */
    CLOG_FILTER_ACCEPT, /* accept and stop to proceed remaining filter chain */
    CLOG_FILTER_REJECT, /* reject and the log will be abandoned */
} clog_filter_res_e;

typedef clog_filter_res_e (*clog_filter_f)(const clog_item_wrapper_t* item);

typedef struct clog_filter clog_filter_t;

struct clog_filter {
    const char* name; /* filter name, it should be the same as the filter defined in config file */
    uint32_t priority; /* filter priority, if same, the order defined in the configuration file will be applied */
    clog_filter_type_e type; /* pre-filter or post-filter */
    clog_filter_f filter; /* filter function */
    const clog_filter_t* next;
};

/**
 * register customized filter
 * @param name filter name, it will be copied
 * @param type #clog_filter_type_e
 * @param filter #clog_filter_f
 * @return #clog_res_e
 */
clog_res_e clog_filter_register(const char* name, clog_filter_type_e type, clog_filter_f filter);

/**
 * filter setup
 * @return clog_res_e
 */
clog_res_e clog_filter_setup(void);

/**
 * cleanup filter resource
 */
void clog_filter_cleanup(void);

/**
 * free filter chain
 * @param filter filter chain
 */
void clog_filter_free(clog_filter_t* filter);

/**
 * execute filter
 * @param filters filter chain
 * @param wrapper log item
 * @return true if item is allowed to be output, false otherwise
 */
bool clog_filter_log(const clog_filter_t* filters, const clog_item_wrapper_t* wrapper);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_FILTER_H */
