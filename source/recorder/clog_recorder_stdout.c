/**
 * @auther polaris
 * @date  2025/10/16
 */
#include "clog_recorder_stdout.h"

#include <stdio.h>

#include "clog_recorder_manager.h"

static clog_res_e clog_recorder_stdout_write(clog_recorder_t* self, const clog_item_t* log)
{
    CLOG_IGNORE_RES(printf("%s", log->content));
    return CLOG_SUCCESS;
}

const clog_recorder_t* clog_recorder_stdout(void)
{
    static const clog_recorder_t tmp = {
        .id = CLOG_RECORDER_STDOUT_ID,
        .setup = clog_recorder_empty_setup,
        .open = clog_recorder_empty_open,
        .write = clog_recorder_stdout_write,
        .flush = clog_recorder_empty_flush,
        .close = clog_recorder_empty_close,
        .cleanup = clog_recorder_empty_cleanup,
        .extra = NULL,
    };
    return &tmp;
}
