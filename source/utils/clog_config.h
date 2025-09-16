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
 * @param data config file, there must be an end char '\0', nonnull
 * @param err buffer to save error message, nullable
 * @param size buffer size
 * @return #clog_config_group_t if success, NULL otherwise
 */
clog_config_group_t* clog_config_parse(const char* data, char* err, size_t size);

/**
 * get item from group, if groups is empty, item will be searched in root group
 * @param root root group
 * @param groups group names, nullable, e.g. ["Performance", "Memory"] means find item in Performance.Memory
 * @param count group count
 * @param key item key, nonnull
 * @return item if success, NULL otherwise
 */
const clog_config_item_t* clog_config_find_item(const clog_config_group_t* root, const char* groups[], size_t count,
                                                const char* key);

/**
 * destroy group
 * @param group group to be destroyed, nonnull
 */
void clog_config_destroy_group(clog_config_group_t* group);

/**
 * dump group that parsed from config file
 * @param buf buffer to save dumped string, nonnull
 * @param size buffer size
 * @return true if success, false otherwise
 */
bool clog_config_dump_group(const clog_config_group_t* group, char* buf, size_t size);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CONFIG_H */
