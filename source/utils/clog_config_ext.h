/**
 * @author polaris
 * @date  2025/10/19
 */
#ifndef CASCA_LOG_CLOG_CONFIG_EXT_H
#define CASCA_LOG_CLOG_CONFIG_EXT_H

#include "clog_config.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * get "enabled" value in group
 * @param group group
 * @param enabled result, if "enabled" not found, #enabled will not be changed
 * @return return CLOG_SUCCESS if succeed or "enabled" not found, #clog_res_e otherwise
 */
clog_res_e clog_config_item_get_enabled(const clog_config_group_t* group, bool* enabled);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CONFIG_EXT_H */
