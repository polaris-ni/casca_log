/**
 * @author Polaris
 * @date  2025/9/3
 */

#include "casca_log.h"
#include <stdio.h>
#include "clog_config.h"
#include "clog_hooks.h"
#include "clog_mem_pool.h"
#include "clog_placeholder.h"
#include "clog_secure_func.h"

static clog_context_t g_context = {0};

static char g_err[CASCA_LOG_ERR_BUF_SIZE] = {0};

static clog_res_e clog_format_init_level_tag(const clog_config_group_t* group, const char* tag, const char** dest)
{
    const clog_config_item_t* item = clog_config_find_item_in_group(group, tag);
    CLOG_RET_IF_NULL_X(item, CLOG_TARGET_NOT_FOUND, "Formatter.Level.tag.%s not found", tag);
    CLOG_RET_IF_X(item->type != CLOG_CONFIG_ITEM_TYPE_STRING, CLOG_INVALID_PARAM,
                  "Formatter.Level.tag.trace type %u error", item->type);
    *dest = clog_strdup(item->value.str);
    CLOG_RET_IF_NULL_X(*dest, CLOG_NO_MEMORY, "clog_strdup tag str [%s] failed", item->value.str);
    return CLOG_SUCCESS;
}

/**
 * this function will parse [Formatter.format]
 * if customized placeholder is used, please call [clog_placeholder_register] before [clog_init]
 * @param root config group root
 * @return #clog_res_e
 */
static clog_res_e clog_formatter_init(const clog_config_group_t* root)
{
    const char* groups[] = {"Formatter"};
    const clog_config_item_t* item = clog_config_find_item(root, groups, CLOG_ARRAY_SIZE(groups), "format");
    CLOG_RET_IF_NULL_X(item, CLOG_TARGET_NOT_FOUND, "item \"format\" of group [Formatter] not found");
    CLOG_RET_IF_X(item->type != CLOG_CONFIG_ITEM_TYPE_STRING, CLOG_ERROR_FORMAT,
                  "format type error, CLOG_CONFIG_ITEM_TYPE_STRING expected, but %u found", item->type);
    CLOG_RET_IF_NULL_X(item->value.str, CLOG_ERROR_FORMAT, "format string is NULL");
    clog_res_e ret = clog_placeholder_parse(item->value.str, &g_context.formatter.placeholder);
    CLOG_RET_IF(ret != CLOG_SUCCESS, ret);

    const char* tag_group[] = {"Formatter", "Level", "Tag"};
    const clog_config_group_t* group = clog_config_find_group(root, tag_group, CLOG_ARRAY_SIZE(tag_group));
    CLOG_RET_IF_NULL_X(group, CLOG_TARGET_NOT_FOUND, "Formatter.Level.tag not found");
    const char* tag[] = {"trace", "debug", "info", "warn", "error", "fetal"};
    const char** dest[] = {&g_context.formatter.level.tag.trace};
    ret = clog_format_init_level_tag(group, "trace", &g_context.formatter.level.tag.trace);
    CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    ret = clog_format_init_level_tag(group, "debug", &g_context.formatter.level.tag.debug);
    CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    ret = clog_format_init_level_tag(group, "info", &g_context.formatter.level.tag.info);
    CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    ret = clog_format_init_level_tag(group, "warn", &g_context.formatter.level.tag.warn);
    CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    ret = clog_format_init_level_tag(group, "error", &g_context.formatter.level.tag.error);
    CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    ret = clog_format_init_level_tag(group, "fetal", &g_context.formatter.level.tag.fetal);
    CLOG_RET_IF(ret != CLOG_SUCCESS, ret);
    return CLOG_SUCCESS;
}

clog_res_e clog_init(const char* process, const char* config)
{
    CLOG_RET_IF_NULL_X(process, CLOG_INVALID_PARAM, "process is NULL");
    CLOG_RET_IF_NULL_X(config, CLOG_INVALID_PARAM, "config is NULL");
    g_context.process = clog_strdup(process);
    if (g_context.process == NULL) {
        return CLOG_NO_MEMORY;
    }

    g_context.config.root = clog_config_parse(config);
    clog_res_e ret = CLOG_ERROR_FORMAT;
    if (g_context.config.root == NULL) {
        goto CLOG_LABEL_CLEAR;
    }

    ret = clog_formatter_init(g_context.config.root);
    if (ret != CLOG_SUCCESS) {
        goto CLOG_LABEL_CLEAR;
    }

#ifdef CASCA_LOG_MEM_POOL
    clog_mp_init(CLOG_MP_PRE_ALLOCATED_NORMAL);
#endif
    return CLOG_SUCCESS;

CLOG_LABEL_CLEAR:
    clog_destroy();
    return ret;
}

const clog_config_group_t* clog_get_config_root(void)
{
    return g_context.config.root;
}

const char* clog_get_process(void)
{
    return g_context.process == NULL ? "NULL" : g_context.process;
}

const char* clog_get_level_tag(clog_level_e level)
{
    switch (level) {
        case CLOG_LEVEL_TRACE:
            return g_context.formatter.level.tag.trace;
        case CLOG_LEVEL_DEBUG:
            return g_context.formatter.level.tag.debug;
        case CLOG_LEVEL_INFO:
            return g_context.formatter.level.tag.info;
        case CLOG_LEVEL_WARN:
            return g_context.formatter.level.tag.warn;
        case CLOG_LEVEL_ERROR:
            return g_context.formatter.level.tag.error;
        case CLOG_LEVEL_FATAL:
            return g_context.formatter.level.tag.fetal;
        default:
            return "NULL";
    }
}

void clog_destroy(void)
{
    CLOG_SAFE_FREE(g_context.process);
    clog_config_destroy_group(g_context.config.root);
    clog_placeholder_clear(&g_context.formatter.placeholder);
    clog_mp_finalize();
    g_context.config.root = NULL;
    g_context.config.level = CLOG_LEVEL_OFF;
}

void clog_err_clear(void)
{
    g_err[0] = '\0';
}

void clog_err_set(const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(fmt);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(g_err, sizeof(g_err), fmt, args);
    va_end(args);
    if (ret <= 0) {
        clog_err_clear();
    }
}

void clog_err_append(const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(fmt);
    const size_t len = strlen(g_err);
    CLOG_RET_VOID_IF(len >= sizeof(g_err) - 1);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(g_err + len, sizeof(g_err) - len, fmt, args);
    va_end(args);
    if (ret <= 0) {
        g_err[len - 1] = '\0';
    }
}

void clog_err_append_line(const char* fmt, ...)
{
    CLOG_RET_VOID_IF_NULL(fmt);
    const size_t len = strlen(g_err);
    CLOG_RET_VOID_IF(len >= sizeof(g_err) - 1);
    va_list args;
    va_start(args, fmt);
    const int ret = vsnprintf(g_err + len, sizeof(g_err) - len, fmt, args);
    va_end(args);
    if (ret <= 0) {
        g_err[len - 1] = '\0';
    } else {
        CLOG_RET_VOID_IF(len + ret >= sizeof(g_err));
        g_err[len + ret] = '\n';
        g_err[len + ret + 1] = '\0';
    }
}

const char* clog_err_get(void)
{
    return g_err;
}
