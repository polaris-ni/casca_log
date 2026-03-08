/**
 * @auther polaris
 * @date  2025/10/16
 */
#include "clog_recorder_stdout.h"
#include <stdio.h>
#include "clog_config_ext.h"
#include "clog_error.h"
#include "clog_hooks.h"
#include "clog_secure_func.h"

typedef struct clog_recorder_stdout_param {
    char *colors[CLOG_LEVEL_NUM];
} clog_recorder_stdout_param_t;

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

static clog_res_e clog_recorder_stdout_open(clog_recorder_t *self)
{
    CLOG_UNUSED_VAR(self);
    return CLOG_SUCCESS;
}

static clog_res_e clog_recorder_stdout_write(clog_recorder_t *self, const clog_item_t *log)
{
    const clog_recorder_stdout_param_t *param = self->extra;
    if (param != NULL && param->colors[log->level - 1] != NULL) {
        CLOG_IGNORE_RES(printf("%s%s\033[0m", param->colors[log->level - 1], log->content));
    } else {
        CLOG_IGNORE_RES(printf("%s", log->content));
    }
    return CLOG_SUCCESS;
}

static void clog_recorder_stdout_close(clog_recorder_t *self)
{
    clog_recorder_stdout_param_t *param = self->extra;
    CLOG_RET_VOID_IF_NULL(param);
    for (size_t i = 0; i < CLOG_LEVEL_NUM; ++i) {
        CLOG_SAFE_FREE(param->colors[i]);
    }
    CLOG_SAFE_FREE(self->extra);
}

static clog_res_e clog_stdout_parse_color_basic(const clog_recorder_stdout_color_value_t *value, char **color,
                                                bool is_enhanced)
{
    char tmp[64] = {0};
    size_t offset = 0;
    if (value->foreground_color != CLOG_COLOR_UNSPECIFIED) {
        CLOG_RET_IF_X(!clog_stdout_check_basic_color(value->foreground_color, is_enhanced, false), CLOG_INVALID_PARAM,
                      "basic color foreground value %u is invalid", value->foreground_color);
        const int ret = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[%um", value->foreground_color);
        CLOG_RET_IF_X(ret <= 0, CLOG_FAIL, "snprintf color %u failed, ret = %d", value->foreground_color, ret);
        offset += ret;
    }
    if (value->background_color != CLOG_COLOR_UNSPECIFIED) {
        CLOG_RET_IF_X(!clog_stdout_check_basic_color(value->background_color, is_enhanced, true), CLOG_INVALID_PARAM,
                      "basic color background value %u is invalid", value->background_color);
        const int ret = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[%um", value->background_color);
        CLOG_RET_IF_X(ret <= 0, CLOG_FAIL, "snprintf color %u failed, ret = %d", value->background_color, ret);
    }
    *color = clog_strdup(tmp);
    CLOG_RET_IF_NULL_X(*color, CLOG_NO_MEMORY, "clog_strdup %s failed", tmp);
    return CLOG_SUCCESS;
}

static clog_res_e clog_stdout_parse_color_256(const clog_recorder_stdout_color_value_t *value, char **color)
{
    char tmp[64] = {0};
    size_t offset = 0;
    if (value->foreground_color != CLOG_COLOR_UNSPECIFIED) {
        const int ret = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[38;5;%um", value->foreground_color);
        CLOG_RET_IF_X(ret <= 0, CLOG_FAIL, "snprintf color %u failed, ret = %d", value->foreground_color, ret);
        offset += ret;
    }
    if (value->background_color != CLOG_COLOR_UNSPECIFIED) {
        const int ret = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[48;5;%um", value->background_color);
        CLOG_RET_IF_X(ret <= 0, CLOG_FAIL, "snprintf color %u failed, ret = %d", value->background_color, ret);
    }

    *color = clog_strdup(tmp);
    CLOG_RET_IF_NULL_X(*color, CLOG_NO_MEMORY, "clog_strdup %s failed", tmp);
    return CLOG_SUCCESS;
}

static clog_res_e clog_stdout_parse_color_true(const clog_recorder_stdout_color_value_t *value, char **color)
{
    char tmp[64] = {0};
    size_t offset = 0;
    if (value->foreground_color != CLOG_COLOR_UNSPECIFIED) {
        const int num = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[38;2;%d;%d;%dm",
                                 CLOG_COLOR_GET_R(value->foreground_color), CLOG_COLOR_GET_G(value->foreground_color),
                                 CLOG_COLOR_GET_B(value->foreground_color));
        CLOG_RET_IF_X(num <= 0, CLOG_FAIL, "snprintf fg color 0x%06X failed, ret = %d", value->foreground_color, num);
        offset += num;
    }
    if (value->background_color != CLOG_COLOR_UNSPECIFIED) {
        const int num = snprintf(tmp + offset, sizeof(tmp) - offset, "\033[48;2;%d;%d;%dm",
                                 CLOG_COLOR_GET_R(value->background_color), CLOG_COLOR_GET_G(value->background_color),
                                 CLOG_COLOR_GET_B(value->background_color));
        CLOG_RET_IF_X(num <= 0, CLOG_FAIL, "snprintf bg color 0x%06X failed, ret = %d", value->background_color, num);
    }
    *color = clog_strdup(tmp);
    CLOG_RET_IF_NULL_X(*color, CLOG_NO_MEMORY, "clog_strdup %s failed", *color);
    return CLOG_SUCCESS;
}

static clog_res_e clog_recorder_stdout_parse_color(clog_stdout_color_mode_e mode,
                                                   const clog_recorder_stdout_color_value_t *value, char **color)
{
    if (mode == CLOG_RECORDER_STDOUT_COLOR_BASIC) {
        return clog_stdout_parse_color_basic(value, color, false);
    }
    if (mode == CLOG_RECORDER_STDOUT_COLOR_BASIC_ENHANCED) {
        return clog_stdout_parse_color_basic(value, color, true);
    }
    if (mode == CLOG_RECORDER_STDOUT_COLOR_256) {
        return clog_stdout_parse_color_256(value, color);
    }
    if (mode == CLOG_RECORDER_STDOUT_COLOR_TRUE) {
        return clog_stdout_parse_color_true(value, color);
    }
    return CLOG_INVALID_PARAM;
}

static clog_recorder_stdout_param_t *clog_recorder_stdout_attr_parse(const clog_recorder_stdout_attr_t *attr)
{
    clog_recorder_stdout_param_t *param = clog_malloc(sizeof(clog_recorder_stdout_param_t));
    CLOG_RET_IF_NULL_X(param, NULL, "malloc clog_recorder_stdout_param_t failed");
    CLOG_IGNORE_RES(clog_memset(param, sizeof(clog_recorder_stdout_param_t), 0, sizeof(clog_recorder_stdout_param_t)));
    CLOG_RET_IF(attr->mode == CLOG_RECORDER_STDOUT_COLOR_OFF, param);
    for (size_t i = 0; i < CLOG_ARRAY_SIZE(param->colors); ++i) {
        const clog_res_e ret = clog_recorder_stdout_parse_color(attr->mode, &attr->colors[i], &param->colors[i]);
        if (ret != CLOG_SUCCESS) {
            for (int j = 0; j < i; ++j) {
                CLOG_SAFE_FREE(param->colors[j]);
            }
            CLOG_SAFE_FREE(param);
            CLOG_ERR_ADD("clog_recorder_stdout_attr_parse failed, index = %u, ret = %u", i, ret);
            return NULL;
        }
    }
    return param;
}

clog_recorder_t *clog_recorder_stdout_create(const clog_recorder_stdout_attr_t *attr)
{
    CLOG_RET_IF_X((attr != NULL) && (attr->mode >= CLOG_RECORDER_STDOUT_COLOR_MAX), NULL, "color mode %u error",
                  attr->mode);
    clog_recorder_t *recorder = clog_malloc(sizeof(clog_recorder_t));
    CLOG_RET_IF_NULL_X(recorder, NULL, "malloc clog_recorder_t failed");
    recorder->id = CLOG_RECORDER_ID_STDOUT;
    recorder->open = clog_recorder_stdout_open;
    recorder->write = clog_recorder_stdout_write;
    recorder->flush = clog_recorder_empty_flush;
    recorder->close = clog_recorder_stdout_close;
    if (attr == NULL || attr->mode == CLOG_RECORDER_STDOUT_COLOR_OFF) {
        recorder->extra = NULL;
        return recorder;
    }
    recorder->extra = clog_recorder_stdout_attr_parse(attr);
    CLOG_CLEAN_RET_IF_NULL_X(recorder->extra, clog_free(recorder), NULL, "clog_recorder_stdout_attr_parse failed");
    return recorder;
}
