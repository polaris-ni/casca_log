/**
 * @author Polaris
 * @date 2026/2/23
 */

#include "clog_recorder_file.h"
#include "clog_recorder_manager.h"

const clog_recorder_t *clog_recorder_file(void) {
    static const clog_recorder_t recorder = {
        .id = CLOG_RECORDER_ID_FILE,
        .open = clog_recorder_empty_open,
        .write = clog_recorder_empty_write,
        .flush = clog_recorder_empty_flush,
        .close = clog_recorder_empty_close,
    };
    return &recorder;
}