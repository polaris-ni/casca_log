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
    return clog_placeholder_parse(item->value.str, &g_context.formatter.placeholder);
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
        goto CLOG_LEBAL_CLEAR;
    }

    ret = clog_formatter_init(g_context.config.root);
    if (ret != CLOG_SUCCESS) {
        goto CLOG_LEBAL_CLEAR;
    }

#ifdef CASCA_LOG_MEM_POOL
    clog_mp_init(CLOG_MP_PRE_ALLOCATED_NORMAL);
#endif
    return CLOG_SUCCESS;

CLOG_LEBAL_CLEAR:
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
