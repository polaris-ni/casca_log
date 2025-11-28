/**
 * @author Polaris
 * @date  2025/9/3
 */
#ifndef CASCA_LOG_CASCA_LOG_CONFIGS_H
#define CASCA_LOG_CASCA_LOG_CONFIGS_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CASCA_LOG_DEBUG 1 /* 0 means debug mode disabled, other values mean enabled */

#define CLOG_HOOK_MODE_DISABLED 0 /* hook is disabled */
#define CLOG_HOOK_MODE_DEBUG_ONLY 1 /* hook is only enabled when CASCA_LOG_DEBUG defined */
#define CLOG_HOOK_MODE_ALWAYS 2 /* hook is always enabled */
#ifndef CASCA_LOG_HOOK_MODE
#ifdef CASCA_LOG_DEBUG
#define CASCA_LOG_HOOK_MODE CLOG_HOOK_MODE_DEBUG_ONLY
#elif
#define CASCA_LOG_HOOK_MODE CLOG_HOOK_MODE_DISABLED
#endif
#endif
#if CASCA_LOG_HOOK_MODE == CLOG_HOOK_MODE_ALWAYS
#define CASCA_LOG_HOOK_ENABLED 1
#elif CASCA_LOG_HOOK_MODE == CLOG_HOOK_MODE_DEBUG_ONLY
#ifdef CASCA_LOG_DEBUG
#define CASCA_LOG_HOOK_ENABLED 1
#endif
#elif CASCA_LOG_HOOK_MODE == CLOG_HOOK_MODE_DISABLED
/* if CASCA_LOG_HOOK_MODE is CLOG_HOOK_MODE_DISABLED, not define CASCA_LOG_HOOK_ENABLED */
#else
#error "CASCA_LOG_HOOK_MODE is invalid"
#endif

#define CASCA_LOG_MEM_POOL 1 /* enable mem pool to improve memory allocator performance */
#ifndef CASCA_LOG_ERR_BUF_SIZE
#define CASCA_LOG_ERR_BUF_SIZE 256
#endif
#ifndef CASCA_LOG_SINGLE_LOG_MAX_SIZE
#define CASCA_LOG_SINGLE_LOG_MAX_SIZE 256
#endif
#ifndef CASCA_LOG_TARGET_RECORDER_MAX_NUM
#define CASCA_LOG_TARGET_RECORDER_MAX_NUM 8
#endif
#ifndef CASCA_LOG_CACHE_LINE_SIZE
#define CASCA_LOG_CACHE_LINE_SIZE 64
#endif

#ifndef CLOG_FILENAME
#ifndef __FILE_NAME__
static const char* clog_get_filename(const char* fullname, int len)
{
    if (len <= 1) {
        return fullname;
    }
    int i = len - 2;
    while (i >= 0) {
        if (fullname[i] == '/' || fullname[i] == '\\') {
            return fullname + i + 1;
        }
        --i;
    }
    return fullname;
}
#define CLOG_FILENAME clog_get_filename(__FILE__, (int)sizeof(__FILE__))
#else
#define CLOG_FILENAME __FILE_NAME__
#endif
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_CONFIGS_H */
