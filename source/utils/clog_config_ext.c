/**
 * @auther polaris
 * @date  2025/10/19
 */
#include "clog_config_ext.h"
#include "casca_log_keywords.h"
#include "clog_error.h"

clog_res_e clog_config_item_get_enabled(const clog_config_group_t *group, bool *enabled)
{
    const clog_res_e ret = clog_config_find_item_in_group_bool(group, CLOG_STR_ENABLED, enabled);
    if (ret == CLOG_TARGET_NOT_FOUND) {
        return CLOG_SUCCESS;
    }
    if (ret != CLOG_SUCCESS) {
        CLOG_ERR_ADD("get \"enabled\" of %s failed, ret = %d", group == NULL ? "NULL" : group->name, ret);
    }
    return ret;
}
