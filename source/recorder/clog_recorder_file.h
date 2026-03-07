/**
 * @author Polaris
 * @date 2026/2/23
 */

#ifndef CASCA_LOG_CLOG_RECORDER_FILE_H
#define CASCA_LOG_CLOG_RECORDER_FILE_H

#include "clog_recorder.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum clog_recorder_file_split_type {
    CLOG_RECORDER_FILE_SPLIT_NONE, /* no spilt */
    CLOG_RECORDER_FILE_SPLIT_SIZE, /* spilt by size, value(unit: Byte) is the max size of a log file */
    CLOG_RECORDER_FILE_SPLIT_TIME, /* spilt by time, value(unit: Second) is the time interval between two log files */
    CLOG_RECORDER_FILE_SPLIT_NUMBER, /* spilt by num of log items, value(unit: Number) is the max num of log items */
    CLOG_RECORDER_FILE_SPLIT_MAX,
} clog_recorder_file_split_type_e;

typedef struct clog_recorder_file_attr {
    const char *metainfo_path;
    const char *log_dir;
    const char *log_name;
    struct {
        clog_recorder_file_split_type_e type;
        size_t limit;
    } split;
} clog_recorder_file_attr_t;

/**
 * create file recorder
 * @param attr file recorder attr
 * @return #clog_recorder_t
 */
clog_recorder_t *clog_recorder_file_create(const clog_recorder_file_attr_t *attr);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_RECORDER_FILE_H */
