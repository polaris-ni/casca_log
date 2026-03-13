/**
 * @author Polaris
 * @date 2026/3/13
 */

#include "clog_recorder_syslog.h"
#include "clog_error.h"

#ifdef CLOG_PLATFORM_WINDOWS

clog_recorder_t *clog_recorder_syslog_create(const char *ident, int opt, int facility)
{
    CLOG_UNUSED_VAR(ident);
    CLOG_UNUSED_VAR(opt);
    CLOG_UNUSED_VAR(facility);
    CLOG_ERR_ADD("syslog not support on platform");
    return NULL;
}

#else

static clog_res_e clog_recorder_syslog_write(clog_recorder_t *self, const clog_item_t *log)
{
    CLOG_UNUSED_VAR(self);
    static const int LOG_LEVEL_MAP[CLOG_LEVEL_NUM] = {
        /* CLOG_LEVEL_TRACE, CLOG_LEVEL_DEBUG, CLOG_LEVEL_INFO, CLOG_LEVEL_WARN, CLOG_LEVEL_ERROR, CLOG_LEVEL_FETAL */
        LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING, LOG_ERR, LOG_CRIT,
    };
    syslog(LOG_LEVEL_MAP[log->level - 1], "%s", log->content);
    return CLOG_SUCCESS;
}

static void clog_recorder_syslog_close(clog_recorder_t *self)
{
    CLOG_UNUSED_VAR(self);
    closelog();
}

clog_recorder_t *clog_recorder_syslog_create(const char *ident, int opt, int facility)
{
    clog_recorder_t *recorder = clog_malloc(sizeof(clog_recorder_t));
    CLOG_RET_IF_NULL_X(recorder, NULL, "malloc clog_recorder_t failed");
    recorder->id = CLOG_RECORDER_ID_SYSLOG;
    recorder->open = clog_recorder_empty_open;
    recorder->write = clog_recorder_syslog_write;
    recorder->flush = clog_recorder_empty_flush;
    recorder->close = clog_recorder_syslog_close;
    recorder->extra = NULL;
    openlog(ident, opt, facility);
    return recorder;
}

#endif
