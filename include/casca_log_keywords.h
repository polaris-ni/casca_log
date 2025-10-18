/**
 * @author Polaris
 * @date  2025/10/13
 */
#ifndef CASCA_LOG_CASCA_LOG_KEYWORDS_H
#define CASCA_LOG_CASCA_LOG_KEYWORDS_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* attributes */
#define CLOG_STR_ID "id"
#define CLOG_STR_TYPE "type"
#define CLOG_STR_PRIORITY "priority"
#define CLOG_STR_ENABLED "enabled"
#define CLOG_STR_KEYWORDS "keywords"

/* groups */
#define CLOG_STR_FILTERS "Filters"
#define CLOG_STR_BASIC_FILTER "ClogBasicFilter"
#define CLOG_STR_KEYWORDS_FILTER "ClogKeywordsFilter"
#define CLOG_STR_DISPATCHERS "Dispatchers"
#define CLOG_STR_DISPATCHER_DIRECT "ClogDirectDispatcher"
#define CLOG_STR_RECORDERS "Recorders"
#define CLOG_STR_RECORDER_STDOUT "ClogRecorderStdout"

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_KEYWORDS_H */
