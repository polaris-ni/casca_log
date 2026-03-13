/**
 * @author Polaris
 * @date  2025/9/3
 */
#ifndef CASCA_LOG_CASCA_LOG_CONFIGS_H
#define CASCA_LOG_CASCA_LOG_CONFIGS_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_HOOK_MODE_DISABLED 0 /* hook is disabled */
#define CLOG_HOOK_MODE_DEBUG_ONLY 1 /* hook is only enabled when CASCA_LOG_DEBUG defined */
#define CLOG_HOOK_MODE_ALWAYS 2 /* hook is always enabled */
#ifndef CASCA_LOG_HOOK_MODE
#ifdef CASCA_LOG_DEBUG
#define CASCA_LOG_HOOK_MODE CLOG_HOOK_MODE_DEBUG_ONLY
#else
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

#ifndef CLOG_FILENAME
#ifndef __FILE_NAME__
static const char *clog_get_filename(const char *fullname, int len)
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

#define CASCA_LOG_VERSION_MAJOR 0u
#define CASCA_LOG_VERSION_MINOR 0u
#define CASCA_LOG_VERSION_REVISION 1u
#define CASCA_LOG_VERSION_COMPAT 1u
#define CASCA_LOG_VERSION_OF(major, minor, revision, compat) \
    (((major) << 24) | ((minor) << 16) | ((revision) << 8) | (compat))

#define CASCA_LOG_VERSION_0_0_1_1                                                                      \
    CASCA_LOG_VERSION_OF(CASCA_LOG_VERSION_MAJOR, CASCA_LOG_VERSION_MINOR, CASCA_LOG_VERSION_REVISION, \
                         CASCA_LOG_VERSION_COMPAT)

#define CASCA_LOG_CHECK_COMPATIBILITY(version1, version2) (((version1) & 0xFF) == ((version2) & 0xFF))

#define CASCA_LOG_VERSION CASCA_LOG_VERSION_0_0_1_1

#define CASCA_LOG_MAGIC 0xABABABABu

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CASCA_LOG_CONFIGS_H */
