/**
 * @auther polaris
 * @date  2025/10/15
 */
#include "clog_dispatcher_manager.h"
#include "casca_log.h"
#include "casca_log_keywords.h"
#include "clog_dispatcher_direct.h"
#include "clog_error.h"
#include "clog_secure_func.h"

static clog_dispatcher_t* g_customized_dispatchers = NULL;
static size_t g_customized_dispatcher_num = 0;

static clog_dispatcher_t* g_dispatcher = NULL;

clog_res_e clog_dispatcher_register(const clog_dispatcher_t* dispatchers, size_t num)
{
    CLOG_RET_IF_NULL_X(dispatchers, CLOG_INVALID_PARAM, "customized dispatcher is NULL");
    CLOG_RET_IF_X(num == 0, CLOG_INVALID_PARAM, "customized dispatcher num is 0");
    const size_t size = sizeof(clog_dispatcher_t) * num;
    g_customized_dispatchers = clog_malloc(size);
    CLOG_RET_IF_NULL_X(g_customized_dispatchers, CLOG_NO_MEMORY, "malloc g_customized_dispatchers failed");
    for (size_t i = 0; i < num; ++i) {
        CLOG_IGNORE_RES(clog_memcpy(&g_customized_dispatchers[i], sizeof(clog_dispatcher_t), &dispatchers[i],
                                    sizeof(clog_dispatcher_t)));
        g_customized_dispatchers[i].extra = NULL;
    }
    g_customized_dispatcher_num = num;
    return CLOG_SUCCESS;
}

static const clog_dispatcher_t* clog_dispatcher_get_origin_by_id(const char* name, uint32_t id)
{
    CLOG_RET_IF_X(id == 0, NULL, "the id of dispatcher %s is invalid");
    if (id > CLOG_DISPATCHER_ID_RESERVED) {
        CLOG_RET_IF_NULL_X(g_customized_dispatchers, NULL, "dispatcher %s not found(customized not set)", name);
        for (size_t i = 0; i < g_customized_dispatcher_num; ++i) {
            if (g_customized_dispatchers[i].id == id) {
                return &g_customized_dispatchers[i];
            }
        }
        clog_err_append_line("customized dispatcher %s not found, id = %u", name, id);
        return NULL;
    }
    const clog_dispatcher_provider_f providers[] = {clog_dispatcher_direct};
    const size_t num = CLOG_ARRAY_SIZE(providers);
    CLOG_RET_IF_X(id > num, NULL, "default dispatcher %s(id %u) not supported now", name, id);
    return providers[id - 1]();
}

clog_res_e clog_dispatcher_setup(void)
{
    const clog_config_group_t* root = clog_get_config_root();
    CLOG_RET_IF_NULL_X(root, CLOG_INVALID_PARAM, "root config is NULL");
    const char* names[] = {CLOG_STR_DISPATCHERS};
    const clog_config_group_t* dispatchers = clog_config_find_group(root, names, CLOG_ARRAY_SIZE(names));
    CLOG_RET_IF_X(dispatchers == NULL || dispatchers->child == NULL, CLOG_INVALID_PARAM, "dispatchers not config");

    const clog_config_group_t* item = dispatchers->child;
    while (item != NULL) {
        bool enabled = false;
        clog_res_e ret = clog_config_find_item_in_group_bool(item, CLOG_STR_ENABLED, &enabled);
        enabled = enabled || (ret == CLOG_TARGET_NOT_FOUND);
        const char* name = item->name;
        if (!enabled) {
            CLOG_CLEAN_RET_IF_X(ret != CLOG_TARGET_NOT_FOUND, clog_dispatcher_cleanup(), ret,
                                "get \"enabled\" of %s failed, ret = %d ", name, ret);
            continue;
        }
        if (g_dispatcher != NULL) {
            clog_err_append_line("%s is enabled whiled dispatcher %u has been enabled", g_dispatcher->id, name);
            clog_dispatcher_cleanup();
            return CLOG_ALREADY_EXISTED;
        }
        uint32_t id = CLOG_DISPATCHER_ID_INVALID;
        ret = clog_config_find_item_in_group_uint(item, CLOG_STR_ID, &id);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_dispatcher_cleanup(), "get id of %s failed, ret = %d", item->name, ret);
        const clog_dispatcher_t* recorder = clog_dispatcher_get_origin_by_id(item->name, id);
        CLOG_CLEAN_RET_IF_NULL_X(recorder, clog_dispatcher_cleanup(), CLOG_TARGET_NOT_FOUND, "dispatcher not found");
        g_dispatcher = clog_malloc(sizeof(clog_dispatcher_t));
        CLOG_CLEAN_RET_IF_NULL_X(recorder, clog_dispatcher_cleanup(), CLOG_NO_MEMORY, "dispatcher malloc failed");
        CLOG_IGNORE_RES(clog_memcpy(g_dispatcher, sizeof(clog_dispatcher_t), recorder, sizeof(clog_dispatcher_t)));
        g_dispatcher->extra = NULL;
        ret = g_dispatcher->open(g_dispatcher, item);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_dispatcher_cleanup(), "open dispatcher %s failed, ret = %d", name, ret);
        item = item->sibling;
    }
    CLOG_SAFE_FREE(g_customized_dispatchers);
    g_customized_dispatcher_num = 0;
    return CLOG_SUCCESS;
}

void clog_dispatcher_cleanup(void)
{
    CLOG_SAFE_FREE(g_customized_dispatchers);
    g_customized_dispatcher_num = 0;
    if (g_dispatcher != NULL) {
        g_dispatcher->close(g_dispatcher);
        CLOG_SAFE_FREE(g_dispatcher);
    }
}

clog_res_e clog_dispatch(const uint32_t* recorders, size_t num, const clog_item_t* item)
{
    CLOG_RET_IF_NULL_X(recorders, CLOG_INVALID_PARAM, "target recorders is NULL");
    CLOG_RET_IF_NULL_X(item, CLOG_INVALID_PARAM, "item is NULL");
    CLOG_ASSERT(g_dispatcher != NULL);
    return g_dispatcher->dispatch(g_dispatcher, recorders, num, item);
}
