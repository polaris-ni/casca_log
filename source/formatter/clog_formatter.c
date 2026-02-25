/**
 * @auther Polaris
 * @date  2025/10/1
 */
#include "clog_formatter.h"
#include <stdio.h>
#include "casca_log.h"
#include "clog_config.h"
#include "clog_error.h"
#include "clog_log_format_placeholder.h"
#include "clog_secure_func.h"

static clog_res_e clog_format_init_tags(const clog_config_group_t *group) {
    const char *tags[] = {"trace", "debug", "info", "warn", "error", "fetal"};
    const clog_level_e levels[] = {
        CLOG_LEVEL_TRACE, CLOG_LEVEL_DEBUG, CLOG_LEVEL_INFO,
        CLOG_LEVEL_WARN, CLOG_LEVEL_ERROR, CLOG_LEVEL_FETAL
    };
    CLOG_ASSERT(CLOG_ARRAY_SIZE(tags) == CLOG_ARRAY_SIZE(levels));
    const size_t size = CLOG_ARRAY_SIZE(levels);
    for (size_t i = 0; i < size; i++) {
        const clog_config_item_t *item = clog_config_find_item_in_group(group, tags[i]);
        CLOG_RET_IF_NULL_X(item, CLOG_TARGET_NOT_FOUND, "Formatter.Level.tag.%s not found", tags[i]);
        CLOG_RET_IF_X(item->type != CLOG_CONFIG_TYPE_STRING, CLOG_INVALID_PARAM,
                      "Formatter.Level.tag.trace type %u error", item->type);
        const char *str = clog_strdup(item->value.str);
        CLOG_RET_IF_NULL_X(str, CLOG_NO_MEMORY, "clog_strdup tag str [%s] failed", item->value.str);
        clog_set_level_tag(levels[i], str);
    }

    return CLOG_SUCCESS;
}

clog_res_e clog_formatter_setup(void) {
    const clog_config_group_t *root = clog_get_config_root();
    CLOG_RET_IF_NULL_X(root, CLOG_TARGET_NOT_FOUND, "root group not found");
    const char *groups[] = {"Formatter"};
    const clog_config_item_t *item = clog_config_find_item(root, groups, CLOG_ARRAY_SIZE(groups), "format");
    CLOG_RET_IF_NULL_X(item, CLOG_TARGET_NOT_FOUND, "item \"format\" of group [Formatter] not found");
    CLOG_RET_IF_X(item->type != CLOG_CONFIG_TYPE_STRING, CLOG_ERROR_FORMAT,
                  "format type error, CLOG_CONFIG_ITEM_TYPE_STRING expected, but %u found", item->type);
    CLOG_RET_IF_NULL_X(item->value.str, CLOG_ERROR_FORMAT, "format string is NULL");

    const char *tag_group[] = {"Formatter", "Level", "Tag"};
    const clog_config_group_t *group = clog_config_find_group(root, tag_group, CLOG_ARRAY_SIZE(tag_group));
    CLOG_RET_IF_NULL_X(group, CLOG_TARGET_NOT_FOUND, "Formatter.Level.tag not found");
    clog_res_e ret = clog_format_init_tags(group);
    CLOG_RET_IF_FAILED_X(ret, "init level tags failed, ret = %u", ret);

    clog_hashmap_t *placeholder_map = clog_log_format_placeholder_get_map();
    CLOG_RET_IF_NULL_X(placeholder_map, CLOG_FAIL, "log format placeholder map is NULL");
    clog_interpolator_context_t *context = clog_interpolator_context_create(placeholder_map);
    CLOG_RET_IF_NULL_X(context, CLOG_FAIL, "create interpolator context failed");
    clog_interpolator_t *interpolator = NULL;
    ret = clog_interpolator_parse(context, item->value.str, &interpolator);
    clog_interpolator_context_destroy(context);
    CLOG_RET_IF_FAILED_X(ret, "parse log format[%s] failed, ret = %u", item->value.str, ret);
    clog_set_interpolator(interpolator);
    return CLOG_SUCCESS;
}

clog_res_e clog_format_log(const clog_interpolator_t *interpolator, const clog_item_wrapper_t *wrapper, char *content,
                           size_t size) {
    CLOG_RET_IF_NULL(interpolator, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(wrapper, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(content, CLOG_INVALID_PARAM);
    CLOG_RET_IF(size <= 1, CLOG_OVERSIZE);
    CLOG_IGNORE_RES(clog_strcpy(wrapper->log->process, sizeof(wrapper->log->process), wrapper->process));
    CLOG_IGNORE_RES(clog_strcpy(wrapper->log->module, sizeof(wrapper->log->module), wrapper->module));
    CLOG_IGNORE_RES(clog_strcpy(wrapper->log->filename, sizeof(wrapper->log->filename), wrapper->filename));

    return clog_interpolator_interpolate(interpolator, (void *) wrapper, content, size, NULL);
}

void clog_formatter_cleanup(void) {
    clog_log_format_placeholder_clear();
    clog_interpolator_t *interpolator = clog_get_interpolator();
    clog_set_interpolator(NULL);
    clog_interpolator_clear(&interpolator);

    const char *tag = clog_get_level_tag(CLOG_LEVEL_TRACE);
    clog_set_level_tag(CLOG_LEVEL_TRACE, NULL);
    CLOG_SAFE_FREE(tag);
    tag = clog_get_level_tag(CLOG_LEVEL_DEBUG);
    clog_set_level_tag(CLOG_LEVEL_DEBUG, NULL);
    CLOG_SAFE_FREE(tag);
    tag = clog_get_level_tag(CLOG_LEVEL_INFO);
    clog_set_level_tag(CLOG_LEVEL_INFO, NULL);
    CLOG_SAFE_FREE(tag);
    tag = clog_get_level_tag(CLOG_LEVEL_WARN);
    clog_set_level_tag(CLOG_LEVEL_WARN, NULL);
    CLOG_SAFE_FREE(tag);
    tag = clog_get_level_tag(CLOG_LEVEL_ERROR);
    clog_set_level_tag(CLOG_LEVEL_ERROR, NULL);
    CLOG_SAFE_FREE(tag);
    tag = clog_get_level_tag(CLOG_LEVEL_FETAL);
    clog_set_level_tag(CLOG_LEVEL_FETAL, NULL);
    CLOG_SAFE_FREE(tag);
}
