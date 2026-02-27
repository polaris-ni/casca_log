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
#define CLOG_STR_LEVEL "level"
#define CLOG_STR_TYPE "type"
#define CLOG_STR_PRIORITY "priority"
#define CLOG_STR_ENABLED "enabled"
#define CLOG_STR_KEYWORDS "keywords"
#define CLOG_STR_MODE "mode"
#define CLOG_STR_TRACE "trace"
#define CLOG_STR_DEBUG "debug"
#define CLOG_STR_INFO "info"
#define CLOG_STR_WARN "warn"
#define CLOG_STR_ERROR "error"
#define CLOG_STR_FETAL "fetal"
#define CLOG_STR_RECORDER "recorder"
#define CLOG_STR_AUTO "auto"
#define CLOG_STR_CAPACITY "capacity"
#define CLOG_STR_THRESHOLD "threshold"
#define CLOG_STR_USE "use"
#define CLOG_STR_INDEX "index"
#define CLOG_STR_METAINFO "metainfo"
#define CLOG_STR_DIRECTORY "directory"
#define CLOG_STR_FILE "file"
#define CLOG_STR_SPLIT "split"

/* groups */
#define CLOG_STR_FILTERS "Filters"
#define CLOG_STR_BASIC_FILTER "ClogBasicFilter"
#define CLOG_STR_KEYWORDS_FILTER "ClogKeywordsFilter"
#define CLOG_STR_DISPATCHERS "Dispatchers"
#define CLOG_STR_DISPATCHER_DIRECT "ClogDirectDispatcher"
#define CLOG_STR_CHANNELS "Channels"
#define CLOG_STR_CHANNEL_ATOMIC_QUEUE "CLogAtomicQueue"
#define CLOG_STR_RECORDERS "Recorders"
#define CLOG_STR_RECORDER_STDOUT "ClogRecorderStdout"
#define CLOG_STR_RECORDER_FILE "ClogRecorderFile"
#define CLOG_STR_COLORS "Colors"
#define CLOG_STR_PERFORMANCE "Performance"
#define CLOG_STR_BUFFER_POOL "BufferPool"

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_KEYWORDS_H */
