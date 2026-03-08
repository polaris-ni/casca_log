/**
 * @author Polaris
 * @date 2026/3/8
 */

#ifndef CASCA_LOG_CLOG_FILTER_KEYWORDS_H
#define CASCA_LOG_CLOG_FILTER_KEYWORDS_H

#include "clog_filter.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * create keywords filter
 * @param priority filter priority
 * @param keywords keywords
 * @return #clog_filter_t
 */
clog_filter_t *clog_filter_keywords_create(uint32_t priority, const char *keywords);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_FILTER_KEYWORDS_H */
