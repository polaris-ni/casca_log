/**
 * @author Polaris
 * @date  2025/10/12
 */
#ifndef CASCA_LOG_CLOG_FILTER_H
#define CASCA_LOG_CLOG_FILTER_H

#include <stdbool.h>
#include <stdint.h>
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

typedef clog_filter_res_e (*clog_filter_f)(const clog_item_t* item);

typedef struct clog_filter clog_filter_t;

struct clog_filter {
    uint32_t priority; /* filter priority, if same, the order defined in the configuration file will be applied */
    clog_filter_type_e type; /* pre-filter or post-filter */
    clog_filter_f filter; /* filter function */
    clog_filter_t* next;
};

bool clog_filter_log(const clog_filter_t* filters, const clog_item_t* item);

bool clog_filter_basic(const clog_item_t* item);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_FILTER_H */
