/**
 * @auther polaris
 * @date  2025/10/15
 */
#include "clog_dispatcher_manager.h"
#include "casca_log.h"
#include "casca_log_keywords.h"
#include "clog_dispatcher_async_thread.h"
#include "clog_dispatcher_direct.h"
#include "clog_error.h"
#include "clog_secure_func.h"

static clog_dispatcher_t *g_customized_dispatchers = NULL;
static size_t g_customized_dispatcher_num = 0;

static clog_dispatcher_t g_dispatcher = {CLOG_DISPATCHER_ID_INVALID, NULL, NULL, NULL, NULL};

clog_res_e clog_dispatcher_register(const clog_dispatcher_t *dispatchers, size_t num)
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

static clog_res_e clog_dispatcher_get_origin_by_id(const char *name, uint32_t id, clog_dispatcher_t *dispatcher)
{
    CLOG_RET_IF_X(id == 0, CLOG_INVALID_PARAM, "the id of dispatcher %s is invalid", name);
    if (id > CLOG_DISPATCHER_ID_RESERVED) {
        CLOG_RET_IF_NULL_X(g_customized_dispatchers, CLOG_TARGET_NOT_FOUND,
                           "dispatcher %s not found(customized not set)", name);
        for (size_t i = 0; i < g_customized_dispatcher_num; ++i) {
            if (g_customized_dispatchers[i].id == id) {
                dispatcher->id = g_customized_dispatchers[i].id;
                dispatcher->open = g_customized_dispatchers[i].open;
                dispatcher->notify = g_customized_dispatchers[i].notify;
                dispatcher->close = g_customized_dispatchers[i].close;
                dispatcher->extra = NULL;
                dispatcher->channel = NULL;
                return CLOG_SUCCESS;
            }
        }
        CLOG_ERR_ADD("customized dispatcher %s not found, id = %u", name, id);
        return CLOG_TARGET_NOT_FOUND;
    }
    const clog_dispatcher_provider_f providers[] = {clog_dispatcher_direct, clog_dispatcher_async_thread};
    const size_t num = CLOG_ARRAY_SIZE(providers);
    CLOG_RET_IF_X((id > num) || (providers[id - 1] == NULL), CLOG_TARGET_NOT_FOUND,
                  "default dispatcher %s(id %u) not supported now", name, id);
    providers[id - 1](dispatcher);
    return CLOG_SUCCESS;
}

clog_res_e clog_dispatcher_setup(void)
{
    const clog_config_group_t *root = clog_get_config_root();
    CLOG_RET_IF_NULL_X(root, CLOG_INVALID_PARAM, "root config is NULL");
    const char *names[] = {CLOG_STR_DISPATCHERS};
    const clog_config_group_t *dispatchers = clog_config_find_group(root, names, CLOG_ARRAY_SIZE(names));
    CLOG_RET_IF_X(dispatchers == NULL || dispatchers->child == NULL, CLOG_INVALID_PARAM, "dispatchers not config");

    const clog_config_group_t *item = dispatchers->child;
    while (item != NULL) {
        bool enabled = false;
        clog_res_e ret = clog_config_find_item_in_group_bool(item, CLOG_STR_ENABLED, &enabled);
        enabled = enabled || (ret == CLOG_TARGET_NOT_FOUND);
        const char *name = item->name;
        if (!enabled) {
            CLOG_CLEAN_RET_IF_X((ret != CLOG_SUCCESS), clog_dispatcher_cleanup(), ret,
                                "get \"enabled\" of %s failed, ret = %d ", name, ret);
            item = item->sibling;
            continue;
        }
        uint32_t id = CLOG_DISPATCHER_ID_INVALID;
        ret = clog_config_find_item_in_group_uint(item, CLOG_STR_ID, &id);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_dispatcher_cleanup(), "get id of %s failed, ret = %d", item->name, ret);
        ret = clog_dispatcher_get_origin_by_id(item->name, id, &g_dispatcher);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_dispatcher_cleanup(), "set dispatcher failed, ret = %u", ret);
        g_dispatcher.channel = clog_get_channel();
        CLOG_CLEAN_RET_IF_NULL_X(g_dispatcher.channel, clog_dispatcher_cleanup(), CLOG_INVALID_PARAM,
                                 "channel is NULL");
        ret = g_dispatcher.open(&g_dispatcher, item);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_dispatcher_cleanup(), "open dispatcher %s failed, ret = %d", name, ret);
        clog_dispatcher_notify(CLOG_DISPATCHER_EVENT_START);
        return CLOG_SUCCESS;
    }
    CLOG_SAFE_FREE(g_customized_dispatchers);
    g_customized_dispatcher_num = 0;
    return CLOG_NOT_SUPPORTED;
}

void clog_dispatcher_notify(clog_dispatcher_event_e event)
{
    if (g_dispatcher.notify != NULL) {
        g_dispatcher.notify(&g_dispatcher, event);
    }
}

void clog_dispatcher_cleanup(void)
{
    CLOG_SAFE_FREE(g_customized_dispatchers);
    g_customized_dispatcher_num = 0;
    clog_dispatcher_notify(CLOG_DISPATCHER_EVENT_END);
    if (g_dispatcher.close != NULL) {
        g_dispatcher.close(&g_dispatcher);
    }
}
