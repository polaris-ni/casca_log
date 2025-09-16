/**
 * @auther Polaris
 * @date  2025/9/12
 */
#ifndef CASCA_LOG_CLOG_CONFIG_H
#define CASCA_LOG_CLOG_CONFIG_H

#include "casca_log_defines.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * parse config file to #clog_config_group_t
 * @param data config file, there must be an end char '\0'
 * @param err buffer to save error message
 * @param size buffer size
 * @return #clog_config_group_t if success, NULL otherwise
 */
clog_config_group_t* clog_config_parse(const char* data, char* err, size_t size);

/**
 * destroy group
 * @param group group to be destroyed, nullable
 */
void clog_config_destroy_group(clog_config_group_t* group);

/**
 * dump group that parsed from config file
 * @param ctx context
 * @param buf buffer to save dumped string
 * @param size buffer size
 * @return true if success, false otherwise
 */
bool clog_config_dump_group(const clog_config_group_t* group, char* buf, size_t size);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CONFIG_H */
