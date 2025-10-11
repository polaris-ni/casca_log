/**
 * @auther Polaris
 * @date  2025/10/1
 */
#include "clog_formatter.h"

#include <stdio.h>

#include "casca_log.h"
#include "clog_config.h"
#include "clog_placeholder.h"
#include "clog_secure_func.h"

static clog_res_e clog_format_init_tags(const clog_config_group_t* group)
{
    const char* tags[] = {"trace", "debug", "info", "warn", "error", "fetal"};
    const clog_level_e levels[] = {CLOG_LEVEL_TRACE, CLOG_LEVEL_DEBUG, CLOG_LEVEL_INFO,
                                   CLOG_LEVEL_WARN,  CLOG_LEVEL_ERROR, CLOG_LEVEL_FETAL};
    CLOG_ASSERT(CLOG_ARRAY_SIZE(tags) == CLOG_ARRAY_SIZE(levels));
    const size_t size = CLOG_ARRAY_SIZE(levels);
    for (size_t i = 0; i < size; i++) {
        const clog_config_item_t* item = clog_config_find_item_in_group(group, tags[i]);
        CLOG_RET_IF_NULL_X(item, CLOG_TARGET_NOT_FOUND, "Formatter.Level.tag.%s not found", tags[i]);
        CLOG_RET_IF_X(item->type != CLOG_CONFIG_ITEM_TYPE_STRING, CLOG_INVALID_PARAM,
                      "Formatter.Level.tag.trace type %u error", item->type);
        const char* str = clog_strdup(item->value.str);
        CLOG_RET_IF_NULL_X(str, CLOG_NO_MEMORY, "clog_strdup tag str [%s] failed", item->value.str);
        clog_set_level_tag(levels[i], str);
    }

    return CLOG_SUCCESS;
}

clog_res_e clog_formatter_setup(void)
{
    const clog_config_group_t* root = clog_get_config_root();
    CLOG_RET_IF_NULL_X(root, CLOG_TARGET_NOT_FOUND, "root group not found");
    const char* groups[] = {"Formatter"};
    const clog_config_item_t* item = clog_config_find_item(root, groups, CLOG_ARRAY_SIZE(groups), "format");
    CLOG_RET_IF_NULL_X(item, CLOG_TARGET_NOT_FOUND, "item \"format\" of group [Formatter] not found");
    CLOG_RET_IF_X(item->type != CLOG_CONFIG_ITEM_TYPE_STRING, CLOG_ERROR_FORMAT,
                  "format type error, CLOG_CONFIG_ITEM_TYPE_STRING expected, but %u found", item->type);
    CLOG_RET_IF_NULL_X(item->value.str, CLOG_ERROR_FORMAT, "format string is NULL");

    const char* tag_group[] = {"Formatter", "Level", "Tag"};
    const clog_config_group_t* group = clog_config_find_group(root, tag_group, CLOG_ARRAY_SIZE(tag_group));
    CLOG_RET_IF_NULL_X(group, CLOG_TARGET_NOT_FOUND, "Formatter.Level.tag not found");
    clog_res_e ret = clog_format_init_tags(group);
    CLOG_RET_IF(ret != CLOG_SUCCESS, ret);

    clog_placeholder_t placeholder = {.func = NULL, .name = NULL, .next = NULL};
    ret = clog_placeholder_parse(item->value.str, &placeholder);
    CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    clog_set_placeholders(placeholder.next);
    return CLOG_SUCCESS;
}

clog_res_e clog_format_log(const clog_placeholder_t* placeholders, const clog_item_t* item, char* content, size_t size)
{
    CLOG_RET_IF_NULL(placeholders, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(item, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(content, CLOG_INVALID_PARAM);
    CLOG_RET_IF(size == 0, CLOG_OVERSIZE);
    const size_t remain = size - 1;
    size_t offset = 0;
    const clog_placeholder_t* placeholder = placeholders;
    while (placeholder != NULL) {
        if (placeholder->func == NULL) {
            const int ret = snprintf(content + offset, remain - offset, "%s", placeholder->name);
            CLOG_RET_IF(ret < 0, CLOG_OVERSIZE);
            offset += ret;
        } else {
            const size_t len = placeholder->func(item, content + offset, remain - offset);
            CLOG_RET_IF(len == 0, CLOG_OVERSIZE);
            offset += len;
        }
        CLOG_RET_IF(offset >= remain, CLOG_OVERSIZE);
        placeholder = placeholder->next;
    }
    content[offset] = '\0'; /* ensure there is an end char */
    return CLOG_SUCCESS;
}

void clog_formatter_cleanup(void)
{
    const clog_placeholder_t* placeholder = clog_get_placeholders();
    clog_set_placeholders(NULL);
    clog_placeholder_clear((clog_placeholder_t*)placeholder);
    CLOG_SAFE_FREE(placeholder);

    const char* tag = clog_get_level_tag(CLOG_LEVEL_TRACE);
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
