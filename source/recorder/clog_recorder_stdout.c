/**
 * @auther polaris
 * @date  2025/10/16
 */
#include "clog_recorder_stdout.h"
#include <stdio.h>
#include "casca_log_keywords.h"
#include "clog_config_ext.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_recorder_manager.h"
#include "clog_secure_func.h"

typedef enum clog_stdout_color_mode {
    CLOG_RECORDER_STDOUT_COLOR_OFF = 0, /* disabled color */
    CLOG_RECORDER_STDOUT_COLOR_BASIC = 1, /* basic ansi colors */
    CLOG_RECORDER_STDOUT_COLOR_BASIC_ENHANCED = 2, /* basic ansi colors and high intensity colors */
    CLOG_RECORDER_STDOUT_COLOR_256 = 3, /* 256-color mode */
    CLOG_RECORDER_STDOUT_COLOR_TRUE = 4, /* 24-bit - RGB */
    CLOG_RECORDER_STDOUT_COLOR_MAX
} clog_stdout_color_mode_e;

typedef struct clog_recorder_stdout_param {
    char *colors[CLOG_LEVEL_NUM];
} clog_recorder_stdout_param_t;

static void clog_recorder_stdout_close(clog_recorder_t *self)
{
    clog_recorder_stdout_param_t *param = self->extra;
    CLOG_RET_VOID_IF_NULL(param);
    for (size_t i = 0; i < CLOG_LEVEL_NUM; ++i) {
        CLOG_SAFE_FREE(param->colors[i]);
    }
    CLOG_SAFE_FREE(self->extra);
}

static bool clog_stdout_check_basic_color(uint32_t value, bool is_enhanced, bool is_background)
{
    if (!is_background) {
        if (is_enhanced) {
            return (value >= 30 && value <= 37) || (value >= 90 && value <= 97);
        }
        return (value >= 30 && value <= 37);
    }
    if (is_enhanced) {
        return (value >= 40 && value <= 47) || (value >= 100 && value <= 107);
    }
    return (value >= 40 && value <= 47);
}

static clog_res_e clog_stdout_parse_basic_color(const clog_config_item_t *item, char **out, bool is_enhanced)
{
    CLOG_RET_IF_X(item->type != CLOG_CONFIG_TYPE_ARRAY, CLOG_ERROR_FORMAT, "color format of %s is invalid", item->key);
    const clog_config_item_t *fg_item = item->value.array;
    char tmp[64] = {0};
    size_t offset = 0;
    CLOG_RET_IF_X((fg_item == NULL) || (fg_item->type != CLOG_CONFIG_TYPE_ARRAY), CLOG_ERROR_FORMAT,
                  "foreground color format of %s is invalid", item->key);
    if (fg_item->value.array != NULL) {
        const clog_config_item_t *color = fg_item->value.array;
        CLOG_RET_IF_X((color->next != NULL), CLOG_ERROR_FORMAT,
                      "basic color format of %s is invalid(only one value allowed)", item->key);
        const uint32_t value = color->value.uint;
        CLOG_RET_IF_X(color->type != CLOG_CONFIG_TYPE_UINT || !clog_stdout_check_basic_color(value, is_enhanced, false),
                      CLOG_ERROR_FORMAT, "basic color of %s invalid(type %d, value %u)", item->key, color->type, value);
        const int ret = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[%um", value);
        CLOG_RET_IF_X(ret <= 0, CLOG_FAIL, "snprintf color %s failed, ret = %d", item->key, ret);
        offset += ret;
    }
    const clog_config_item_t *bg_item = fg_item->next;
    CLOG_RET_IF_X((bg_item == NULL) || (bg_item->type != CLOG_CONFIG_TYPE_ARRAY), CLOG_ERROR_FORMAT,
                  "background color format of %s is invalid", item->key);
    if (bg_item->value.array != NULL) {
        const clog_config_item_t *color = bg_item->value.array;
        CLOG_RET_IF_X((color->next != NULL), CLOG_ERROR_FORMAT,
                      "basic color format of %s is invalid(only one value allowed)", item->key);
        const uint32_t value = color->value.uint;
        CLOG_RET_IF_X(color->type != CLOG_CONFIG_TYPE_UINT || !clog_stdout_check_basic_color(value, is_enhanced, true),
                      CLOG_ERROR_FORMAT, "basic color of %s invalid(type %d, value %u)", item->key, color->type, value);
        const int ret = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[%um", value);
        CLOG_RET_IF_X(ret <= 0, CLOG_FAIL, "snprintf color %s failed, ret = %d", item->key, ret);
    }
    *out = clog_strdup(tmp);
    CLOG_RET_IF_NULL_X(*out, CLOG_NO_MEMORY, "clog_strdup failed");
    return CLOG_SUCCESS;
}

static clog_res_e clog_stdout_parse_256_color(const clog_config_item_t *item, char **out)
{
    CLOG_RET_IF_X(item->type != CLOG_CONFIG_TYPE_ARRAY, CLOG_ERROR_FORMAT, "color format of %s is invalid", item->key);
    const clog_config_item_t *fg_item = item->value.array;
    char tmp[64] = {0};
    size_t offset = 0;
    CLOG_RET_IF_X((fg_item == NULL) || (fg_item->type != CLOG_CONFIG_TYPE_ARRAY), CLOG_ERROR_FORMAT,
                  "foreground color format of %s is invalid", item->key);
    if (fg_item->value.array != NULL) {
        const clog_config_item_t *color = fg_item->value.array;
        CLOG_RET_IF_X((color->next != NULL), CLOG_ERROR_FORMAT,
                      "256 color format of %s is invalid(only one value allowed)", item->key);
        const uint32_t value = color->value.uint;
        CLOG_RET_IF_X(color->type != CLOG_CONFIG_TYPE_UINT || value > UINT8_MAX, CLOG_ERROR_FORMAT,
                      "256 color of %s invalid(type %d, value %u)", item->key, color->type, value);
        const int ret = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[38;5;%um", value);
        CLOG_RET_IF_X(ret <= 0, CLOG_FAIL, "snprintf color %s failed, ret = %d", item->key, ret);
        offset += ret;
    }
    const clog_config_item_t *bg_item = fg_item->next;
    CLOG_RET_IF_X((bg_item == NULL) || (bg_item->type != CLOG_CONFIG_TYPE_ARRAY), CLOG_ERROR_FORMAT,
                  "background color format of %s is invalid", item->key);
    if (bg_item->value.array != NULL) {
        const clog_config_item_t *color = bg_item->value.array;
        CLOG_RET_IF_X((color->next != NULL), CLOG_ERROR_FORMAT,
                      "256 color format of %s is invalid(only one value allowed)", item->key);
        const uint32_t value = color->value.uint;
        CLOG_RET_IF_X(color->type != CLOG_CONFIG_TYPE_UINT || value > UINT8_MAX, CLOG_ERROR_FORMAT,
                      "256 color of %s invalid(type %d, value %u)", item->key, color->type, value);
        const int ret = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[48;5;%um", value);
        CLOG_RET_IF_X(ret <= 0, CLOG_FAIL, "snprintf color %s failed, ret = %d", item->key, ret);
    }
    *out = clog_strdup(tmp);
    CLOG_RET_IF_NULL_X(*out, CLOG_NO_MEMORY, "clog_strdup failed");
    return CLOG_SUCCESS;
}

static clog_res_e clog_stdout_parse_true_color_rgb(const char *level, const clog_config_item_t *array, uint8_t *r,
                                                   uint8_t *g, uint8_t *b)
{
    const clog_config_item_t *color = array;
    CLOG_RET_IF_X(color->type != CLOG_CONFIG_TYPE_UINT || color->value.uint > UINT8_MAX, CLOG_ERROR_FORMAT,
                  "true color of %s invalid(type %d, value %u)", level, color->type, color->value.uint);
    *r = color->value.uint;
    color = color->next;
    CLOG_RET_IF_X((color == NULL) || (color->type != CLOG_CONFIG_TYPE_UINT) || (color->value.uint > UINT8_MAX),
                  CLOG_ERROR_FORMAT, "true color of %s invalid, value G not configured or invalid", level);
    *g = color->value.uint;
    color = color->next;
    CLOG_RET_IF_X((color == NULL) || (color->type != CLOG_CONFIG_TYPE_UINT) || (color->value.uint > UINT8_MAX),
                  CLOG_ERROR_FORMAT, "true color of %s invalid, value B not configured or invalid", level);
    *b = color->value.uint;
    color = color->next;
    CLOG_RET_IF_X(color != NULL, CLOG_ERROR_FORMAT, "true color of %s invalid, more than 3 values given", level);
    return CLOG_SUCCESS;
}

static clog_res_e clog_stdout_parse_true_color(const clog_config_item_t *item, char **out)
{
    CLOG_RET_IF_X(item->type != CLOG_CONFIG_TYPE_ARRAY, CLOG_ERROR_FORMAT, "color format of %s is invalid", item->key);
    const clog_config_item_t *fg_item = item->value.array;
    char tmp[64] = {0};
    size_t offset = 0;
    CLOG_RET_IF_X((fg_item == NULL) || (fg_item->type != CLOG_CONFIG_TYPE_ARRAY), CLOG_ERROR_FORMAT,
                  "foreground color format of %s is invalid", item->key);
    if (fg_item->value.array != NULL) {
        uint8_t r, g, b;
        const clog_res_e ret = clog_stdout_parse_true_color_rgb(item->key, fg_item->value.array, &r, &g, &b);
        CLOG_RET_IF_FAILED_X(ret, "parse R, G, B of true color failed, ret = %d", ret);
        const int num = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[38;2;%d;%d;%dm", r, g, b);
        CLOG_RET_IF_X(num <= 0, CLOG_FAIL, "snprintf color %s failed, ret = %d", item->key, ret);
        offset += num;
    }
    const clog_config_item_t *bg_item = fg_item->next;
    CLOG_RET_IF_X((bg_item == NULL) || (bg_item->type != CLOG_CONFIG_TYPE_ARRAY), CLOG_ERROR_FORMAT,
                  "background color format of %s is invalid", item->key);
    if (bg_item->value.array != NULL) {
        uint8_t r, g, b;
        const clog_res_e ret = clog_stdout_parse_true_color_rgb(item->key, bg_item->value.array, &r, &g, &b);
        CLOG_RET_IF_FAILED_X(ret, "parse R, G, B of true color failed, ret = %d", ret);
        const int num = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[48;2;%d;%d;%dm", r, g, b);
        CLOG_RET_IF_X(num <= 0, CLOG_FAIL, "snprintf color %s failed, ret = %d", item->key, ret);
    }
    *out = clog_strdup(tmp);
    CLOG_RET_IF_NULL_X(*out, CLOG_NO_MEMORY, "clog_strdup failed");
    return CLOG_SUCCESS;
}

static clog_res_e clog_stdout_parse_single_level(const clog_config_item_t *item, clog_stdout_color_mode_e mode,
                                                 char **out)
{
    if (mode == CLOG_RECORDER_STDOUT_COLOR_BASIC) {
        return clog_stdout_parse_basic_color(item, out, false);
    }
    if (mode == CLOG_RECORDER_STDOUT_COLOR_BASIC_ENHANCED) {
        return clog_stdout_parse_basic_color(item, out, true);
    }
    if (mode == CLOG_RECORDER_STDOUT_COLOR_256) {
        return clog_stdout_parse_256_color(item, out);
    }
    if (mode == CLOG_RECORDER_STDOUT_COLOR_TRUE) {
        return clog_stdout_parse_true_color(item, out);
    }
    return CLOG_INVALID_PARAM;
}

static clog_res_e clog_stdout_parse_colors(const clog_config_group_t *group, clog_recorder_stdout_param_t *param)
{
    uint32_t value = 0;
    clog_res_e ret = clog_config_find_item_in_group_uint(group, CLOG_STR_MODE, &value);
    CLOG_RET_IF_FAILED_X(ret, "get \"mode\" of " CLOG_STR_RECORDER_STDOUT " failed, ret = %d", ret);
    CLOG_RET_IF(value == CLOG_RECORDER_STDOUT_COLOR_OFF, CLOG_SUCCESS);
    CLOG_RET_IF_X(value >= CLOG_RECORDER_STDOUT_COLOR_MAX, CLOG_INVALID_PARAM, "color mode %u is invalid", value);
    const char *name[] = {CLOG_STR_TRACE, CLOG_STR_DEBUG, CLOG_STR_INFO, CLOG_STR_WARN, CLOG_STR_ERROR, CLOG_STR_FETAL};
    for (size_t i = 0; i < CLOG_ARRAY_SIZE(name); ++i) {
        const clog_config_item_t *item = clog_config_find_item_in_group(group, name[i]);
        if (item == NULL) {
            continue;
        }
        ret = clog_stdout_parse_single_level(item, value, &param->colors[i]);
        CLOG_RET_IF_FAILED_X(ret, "parse color of %s failed, ret = %d", item->key, ret);
    }
    return CLOG_SUCCESS;
}

static clog_res_e clog_recorder_stdout_open(clog_recorder_t *self, const clog_config_group_t *group)
{
    clog_recorder_stdout_param_t *param = clog_malloc(sizeof(clog_recorder_stdout_param_t));
    CLOG_RET_IF_NULL_X(param, CLOG_NO_MEMORY, "malloc param failed");
    CLOG_IGNORE_RES(clog_memset(param, sizeof(clog_recorder_stdout_param_t), 0, sizeof(clog_recorder_stdout_param_t)));
    self->extra = param;
    const char *names[] = {CLOG_STR_COLORS};
    const clog_config_group_t *color_group = clog_config_find_group(group, names, CLOG_ARRAY_SIZE(names));
    CLOG_RET_IF_NULL(color_group, CLOG_SUCCESS); /* colors is configured */
    bool enabled = true;
    clog_res_e ret = clog_config_item_get_enabled(color_group, &enabled);
    CLOG_CLEAN_RET_IF_FAILED(ret, clog_recorder_stdout_close(self));
    CLOG_RET_IF(!enabled, CLOG_SUCCESS);
    ret = clog_stdout_parse_colors(color_group, param);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_recorder_stdout_close(self),
                               "parse " CLOG_STR_RECORDER_STDOUT " failed, ret = %d", ret);
    return CLOG_SUCCESS;
}

static clog_res_e clog_recorder_stdout_write(clog_recorder_t *self, const clog_item_t *log)
{
    const clog_recorder_stdout_param_t *param = self->extra;
    if ((param != NULL) && (param->colors[log->level - 1] != NULL)) {
        CLOG_IGNORE_RES(printf("%s%s\033[0m", param->colors[log->level - 1], log->content));
    } else {
        CLOG_IGNORE_RES(printf("%s", log->content));
    }
    return CLOG_SUCCESS;
}

const clog_recorder_t *clog_recorder_stdout(void)
{
    static const clog_recorder_t tmp = {
        .id = CLOG_RECORDER_ID_STDOUT,
        .open = clog_recorder_stdout_open,
        .write = clog_recorder_stdout_write,
        .flush = clog_recorder_empty_flush,
        .close = clog_recorder_stdout_close,
        .extra = NULL,
    };
    return &tmp;
}
