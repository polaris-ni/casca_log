/**
 * @auther Polaris
 * @date  2025/12/6
 */
#include "clog_channel_manager.h"

#include "clog_channel_atomic_queue.h"
#include "clog_error.h"
#include "clog_hooks.h"

typedef struct clog_channel_node clog_channel_node_t;

struct clog_channel_node {
    clog_channel_id_t id;
    clog_channel_provider_f provider;
    clog_channel_node_t* next;
};

static clog_channel_node_t g_customized_channel = {.id = CLOG_CHANNEL_ID_INVALID, .provider = NULL, .next = NULL};

clog_res_e clog_channel_register_provider(clog_channel_id_t id, clog_channel_provider_f provider)
{
    CLOG_RET_IF_X(id <= CLOG_CHANNEL_ID_RESERVED, CLOG_INVALID_PARAM, "invalid channel id(%u)", id);
    clog_channel_node_t* cur = &g_customized_channel;
    while (cur->next != NULL) {
        CLOG_RET_IF_X(cur->next->id == id, CLOG_ALREADY_EXISTED, "channel id(%u) already registered", id);
        cur = cur->next;
    }
    clog_channel_node_t* node = clog_malloc(sizeof(clog_channel_node_t));
    CLOG_RET_IF_NULL_X(node, CLOG_NO_MEMORY, "malloc clog_channel_node_t failed");
    node->id = id;
    node->provider = provider;
    node->next = NULL;
    cur->next = node;
    return CLOG_SUCCESS;
}

static bool clog_channel_check(clog_channel_id_t id, const clog_channel_t* channel)
{
    CLOG_RET_IF_X(channel->id != id, false, "id not match, %u expected, %u received", id, channel->id);
    CLOG_RET_IF_NULL_X(channel->open, false, "channel open function is null");
    CLOG_RET_IF_NULL_X(channel->write, false, "channel write function is null");
    CLOG_RET_IF_NULL_X(channel->read, false, "channel read function is null");
    CLOG_RET_IF_NULL_X(channel->close, false, "channel close function is null");
    return true;
}

static clog_res_e clog_default_channel_get(clog_channel_id_t id, clog_channel_t* channel)
{
    const clog_channel_provider_f providers[] = {clog_atomic_queue_channel_provider};
    const size_t num = CLOG_ARRAY_SIZE(providers);
    CLOG_RET_IF_X(id == CLOG_CHANNEL_ID_INVALID || id > num, CLOG_TARGET_NOT_FOUND, "invalid channel id(%u)", id);
    providers[id - 1](channel);
    return clog_channel_check(id, channel) ? CLOG_SUCCESS : CLOG_INVALID_PARAM;
}

clog_res_e clog_channel_get(clog_channel_id_t id, clog_channel_t* channel)
{
    CLOG_RET_IF_NULL_X(channel, CLOG_INVALID_PARAM, "channel is null");
    if (id < CLOG_CHANNEL_ID_RESERVED) {
        return clog_default_channel_get(id, channel);
    }
    const clog_channel_node_t* node = g_customized_channel.next;
    while (node != NULL) {
        if (node->id == id) {
            node->provider(channel);
            return clog_channel_check(id, channel) ? CLOG_SUCCESS : CLOG_INVALID_PARAM;
        }
        node = node->next;
    }
    return CLOG_TARGET_NOT_FOUND;
}
