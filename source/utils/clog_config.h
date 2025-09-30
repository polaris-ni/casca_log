/**
 * @author Polaris
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
 * @return #clog_config_group_t if success, NULL otherwise
 */
clog_config_group_t* clog_config_parse(const char* data);

/**
 * find group from root
 * @param root root group
 * @param groups group names, nullable, e.g. ["Performance", "Memory"] means find group in Performance.Memory
 * @param count group count
 * @return #clog_config_group_t if found, NULL otherwise, returns root itself if groups is empty
 */
const clog_config_group_t* clog_config_find_group(const clog_config_group_t* root, const char* groups[], size_t count);

/**
 * get item from group
 * @param group group
 * @param key item key, nonnull
 * @return item if success, NULL otherwise
 */
const clog_config_item_t* clog_config_find_item_in_group(const clog_config_group_t* group, const char* key);

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
 * @param group group to be dumped, nonnull
 * @param buf buffer to save dumped string, nonnull
 * @param size buffer size
 * @return true if success, false otherwise
 */
bool clog_config_dump_group(const clog_config_group_t* group, char* buf, size_t size);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CONFIG_H */
