/**
 * @author Polaris
 * @date 2026/3/4
 */

#include "casca_log_core.h"
#include "casca_log_keywords.h"
#include "clog_atomic_types.h"
#include "clog_channel_base.h"
#include "clog_config.h"
#include "clog_config_ext.h"
#include "clog_error.h"
#include "clog_filter.h"
#include "clog_formatter.h"
#include "clog_hashmap.h"
#include "clog_interpolator.h"
#include "clog_log_format_placeholder.h"
#ifndef CASCA_LOG_LOCKLESS
#include "clog_mutex.h"
#endif
#ifdef CASCA_LOG_MEM_POOL
#include "clog_buffer_pool.h"
#endif

#define CLOG_CONTEXT_LOCK() CLOG_IGNORE_RES(clog_mutex_lock(&context->lock))
#define CLOG_CONTEXT_UNLOCK() CLOG_IGNORE_RES(clog_mutex_unlock(&context->lock))

#define CLOG_CONTEXT_SETUP_STATE_CHECK()                        \
    clog_state_e _tmp_state = clog_atomic_get(&context->state); \
    CLOG_RET_IF_X(_tmp_state != CLOG_STATE_CREATED, CLOG_ABNORMAL_STATE, "clog state %u error", _tmp_state);

typedef struct clog_context {
    clog_atomic_type_t state;
#ifndef CASCA_LOG_LOCKLESS
    clog_mutex_t lock;
#endif
#ifdef CASCA_LOG_MEM_POOL
    clog_buffer_pool_t *pool;
#endif
    char process[CASCA_LOG_PROCESS_NAME_MAX_SIZE];
    uint32_t mask;
    clog_hashmap_t *modules;

    struct {
        clog_hashmap_t *placeholders;
        clog_interpolator_context_t *interpolator_context;
        clog_interpolator_t *interpolator;
        const char *tags[CLOG_LEVEL_NUM];
    } formatter;

    struct {
        clog_filter_t pre;
        clog_filter_t post;
    } filters;
    clog_channel_t *channel;
    clog_dispatcher_t *dispatcher;
    clog_hashmap_t *recorders;
} clog_context_t;

static clog_res_e clog_init_level_tags(clog_context_t *context)
{
    for (size_t i = 0; i < CLOG_LEVEL_NUM; i++) {
        const char *default_tags[CLOG_LEVEL_NUM] = {"T", "D", "I", "W", "E", "F"};
        context->formatter.tags[i] = clog_strdup(default_tags[i]);
        if (context->formatter.tags[i] == NULL) {
            CLOG_ERR_ADD("clog_strdup level tag %s failed, index = %zu", default_tags[i], i);
            for (size_t j = 0; j < i; j++) {
                CLOG_SAFE_FREE(context->formatter.tags[i]);
            }
            return CLOG_FAIL;
        }
    }
    return CLOG_SUCCESS;
}

clog_res_e clog_context_create(const char *process, clog_context_t **context)
{
    CLOG_RET_IF_NULL_X(process, CLOG_INVALID_PARAM, "process is NULL");
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    clog_context_t *tmp = clog_malloc(sizeof(clog_context_t));
    CLOG_RET_IF_NULL_X(tmp, CLOG_NO_MEMORY, "malloc clog_context_t failed");
    CLOG_IGNORE_RES(clog_memset(tmp, sizeof(clog_context_t), 0, sizeof(clog_context_t)));
    clog_res_e ret = clog_strcpy(tmp->process, sizeof(tmp->process), process);
    CLOG_RET_IF_FAILED_X(ret, "clog_strcpy process %s failed, ret = %u", process, ret);
#ifndef CASCA_LOG_LOCKLESS
    ret = clog_mutex_init(&tmp->lock);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_free(tmp), "clog_mutex_init failed, ret = %u", ret);
#endif
    ret = clog_init_level_tags(tmp);
    CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_context_destroy(&tmp), "clog_init_level_tags failed, ret = %u", ret);
    tmp->mask = CLOG_LEVEL_ALL;
    clog_atomic_set(&tmp->state, CLOG_STATE_CREATED);
    *context = tmp;
    return CLOG_SUCCESS;
}

void clog_set_mask(clog_context_t *context, uint32_t mask)
{
    CLOG_RET_VOID_IF_NULL_X(context, "context is NULL");
    CLOG_RET_VOID_IF_X(context->state != CLOG_STATE_CREATED, "clog state %u error", context->state);
    context->mask = mask;
}

clog_res_e clog_set_buffer_pool(clog_context_t *context, bool auto_expand, size_t capacity, size_t threshold)
{
#ifdef CASCA_LOG_MEM_POOL
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK();
    CLOG_RET_IF_X(threshold == 0 || threshold > 100, CLOG_INVALID_PARAM, "threshold(%u) is invalid", capacity);
    CLOG_RET_IF_X(context->pool != NULL, CLOG_ALREADY_EXISTED, "clog mem pool has been set");
    clog_buffer_pool_t *pool = NULL;
    const clog_res_e ret = clog_buffer_pool_initialize(&pool, sizeof(clog_item_t), capacity, auto_expand, threshold);
    CLOG_RET_IF_FUNC_FAILED_X(clog_buffer_pool_initialize, ret);
    context->pool = pool;
    return CLOG_SUCCESS;
#else
    CLOG_UNUSED_VAR(context);
    CLOG_UNUSED_VAR(auto_expand);
    CLOG_UNUSED_VAR(capacity);
    CLOG_UNUSED_VAR(threshold);
    CLOG_ERR_ADD("mem pool not supported");
    return CLOG_NOT_SUPPORTED;
#endif
}

static clog_res_e clog_init_process_module(uint32_t default_mask, const char *process,
                                           const clog_config_group_t *module, clog_hashmap_t *map)
{
    bool enabled = true;
    clog_res_e ret = clog_config_item_get_enabled(module, &enabled);
    CLOG_RET_IF_FUNC_FAILED_X(clog_config_item_get_enabled, ret);
    CLOG_RET_IF(!enabled, CLOG_SUCCESS); /* module is not enabled, skip parse */
    uint32_t value = 0;
    ret = clog_config_find_item_in_group_uint(module, CLOG_STR_LEVEL, &value);
    if (ret == CLOG_TARGET_NOT_FOUND) {
        value = default_mask;
    } else {
        CLOG_RET_IF_FAILED_X(ret, "parse level failed in [%s.%s], ret = %u", process, module->name, ret);
    }
    CLOG_RET_IF_X(value > CLOG_LEVEL_ALL, CLOG_INVALID_PARAM, "level(%u) of %s is invalid", module->name, value);
    clog_module_t tmp = {.level = value, .num = 0, .recorders = {0}};
    const clog_config_item_t *recorders = clog_config_find_item_in_group(module, CLOG_STR_RECORDER);
    if (recorders != NULL) {
        CLOG_RET_IF_X(recorders->type != CLOG_CONFIG_TYPE_ARRAY, CLOG_ERROR_FORMAT,
                      "\"recoder\" of module %s should be array", module->name);
        const clog_config_item_t *data = recorders->value.array;
        while (data != NULL) {
            CLOG_RET_IF_X(data->type != CLOG_CONFIG_TYPE_UINT, CLOG_ERROR_FORMAT,
                          "\"recoder\" of module %s should be unsigned int", module->name);
            CLOG_RET_IF_X(tmp.num > CLOG_ARRAY_SIZE(tmp.recorders), CLOG_OVERSIZE,
                          "recoder num of %s oversize, max num is %zu", module->name, CLOG_ARRAY_SIZE(tmp.recorders));
            tmp.recorders[tmp.num] = data->value.uint;
            tmp.num++;
            data = data->next;
        }
    }
    return clog_hashmap_put(map, module->name, &tmp);
}

static clog_res_e clog_init_modules(clog_context_t *context, const clog_config_group_t *root)
{
    const char *process = context->process;
    const char *groups[] = {CLOG_STR_G_PROCESS, process};
    const clog_config_group_t *group = clog_config_find_group(root, groups, CLOG_ARRAY_SIZE(groups));
    CLOG_RET_IF_NULL_X(group, CLOG_TARGET_NOT_FOUND, "process [%s] not found", process);
    if (context->modules == NULL) {
        clog_hashmap_t *map =
            clog_hashmap_create(0, sizeof(clog_module_t), clog_hashmap_string_dup, clog_hashmap_string_free, NULL, NULL,
                                clog_hashmap_string_cmp, clog_hashmap_string_size, 0);
        CLOG_RET_IF_NULL_X(map, CLOG_NO_MEMORY, "clog_hashmap_create failed");
        context->modules = map;
    }

    const clog_config_group_t *child = group->child;
    while (child != NULL) {
        const clog_res_e ret = clog_init_process_module(context->mask, process, child, context->modules);
        CLOG_RET_IF_FUNC_FAILED_X(clog_init_process_module, ret);
        child = child->sibling;
    }
    return CLOG_SUCCESS;
}

clog_res_e clog_set_modules(clog_context_t *context, const char *config)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(config, CLOG_INVALID_PARAM, "config is NULL");
    CLOG_RET_IF_X(context->modules != NULL, CLOG_ALREADY_EXISTED, "modules has been set");
    clog_config_group_t *root = clog_config_parse(config);
    CLOG_RET_IF_NULL_X(root, CLOG_ERROR_FORMAT, "parse config failed");
    const clog_res_e ret = clog_init_modules(context, root);
    clog_config_destroy_group(root);
    CLOG_RET_IF_FUNC_FAILED_X(clog_init_modules, ret);
    return CLOG_SUCCESS;
}

clog_res_e clog_add_module(clog_context_t *context, const clog_module_t *module)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(module, CLOG_INVALID_PARAM, "module is NULL");
    if (context->modules == NULL) {
        clog_hashmap_t *map =
            clog_hashmap_create(0, sizeof(clog_module_t), clog_hashmap_string_dup, clog_hashmap_string_free, NULL, NULL,
                                clog_hashmap_string_cmp, clog_hashmap_string_size, 0);
        CLOG_RET_IF_NULL_X(map, CLOG_NO_MEMORY, "clog_hashmap_create modules failed");
        context->modules = map;
    }
    const clog_res_e ret = clog_hashmap_put(context->modules, module->name, module);
    CLOG_RET_IF_FAILED_X(ret, "clog_hashmap_put module %s failed, ret = %u", module->name, ret);
    return CLOG_SUCCESS;
}

clog_res_e clog_add_modules(clog_context_t *context, const clog_module_t *module, size_t num)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(module, CLOG_INVALID_PARAM, "module is NULL");
    CLOG_RET_IF_X(num == 0, CLOG_INVALID_PARAM, "module num is 0");

    for (size_t i = 0; i < num; ++i) {
        const clog_res_e ret = clog_add_module(context, &module[i]);
        if (ret != CLOG_SUCCESS) {
            CLOG_ERR_ADD("clog_add_modules %s failed, index = %zu, ret = %u", module[i].name, i, ret);
            for (size_t j = 0; j < i; ++j) {
                CLOG_IGNORE_RES(clog_hashmap_remove(context->modules, module[j].name));
            }
            return ret;
        }
    }
    return CLOG_SUCCESS;
}

static clog_res_e clog_init_log_format_interpolator_context(clog_context_t *context)
{
    CLOG_RET_IF(context->formatter.interpolator_context != NULL, CLOG_SUCCESS);
    if (context->formatter.placeholders == NULL) {
        context->formatter.placeholders = clog_log_format_default_placeholder_map_create();
        CLOG_RET_IF_NULL_X(context->formatter.placeholders, CLOG_FAIL, "clog_log_format_placeholder_map_create failed");
    }
    context->formatter.interpolator_context = clog_interpolator_context_create(context->formatter.placeholders);
    CLOG_RET_IF_NULL_X(context->formatter.interpolator_context, CLOG_FAIL, "clog_interpolator_context_create failed");
    return CLOG_SUCCESS;
}

clog_res_e clog_register_log_format_placeholder_handler(clog_context_t *context, const char *name,
                                                        clog_placeholder_handler_f handler)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(name, CLOG_INVALID_PARAM, "placeholder name is NULL");
    CLOG_RET_IF_NULL_X(handler, CLOG_INVALID_PARAM, "placeholder handler is NULL");
    clog_res_e ret = clog_init_log_format_interpolator_context(context);
    CLOG_RET_IF_FUNC_FAILED_X(clog_init_log_format_interpolator_context, ret);
    ret = clog_interpolator_context_register(context->formatter.interpolator_context, name, handler);
    CLOG_RET_IF_FUNC_FAILED_X(clog_interpolator_context_register, ret);
    return CLOG_SUCCESS;
}

clog_res_e clog_set_log_format(clog_context_t *context, const char *format)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(format, CLOG_INVALID_PARAM, "format is NULL");
    if (context->formatter.interpolator != NULL) {
        clog_interpolator_clear(&context->formatter.interpolator);
    }
    clog_res_e ret = clog_init_log_format_interpolator_context(context);
    CLOG_RET_IF_FUNC_FAILED_X(clog_init_log_format_interpolator_context, ret);
    ret = clog_interpolator_parse(context->formatter.interpolator_context, format, &context->formatter.interpolator);
    CLOG_RET_IF_FUNC_FAILED_X(clog_interpolator_parse, ret);
    return CLOG_SUCCESS;
}

clog_res_e clog_set_level_tags(clog_context_t *context, clog_level_e level, const char *tag)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_X(level == CLOG_LEVEL_OFF || level > CLOG_LEVEL_FETAL, CLOG_INVALID_PARAM, "level %u is invalid",
                  level);
    CLOG_RET_IF_NULL_X(tag, CLOG_INVALID_PARAM, "tag is NULL");
    const char *tag_copy = clog_strdup(tag);
    CLOG_RET_IF_NULL_X(tag_copy, CLOG_NO_MEMORY, "clog_strdup %s failed", tag);
    CLOG_SAFE_FREE(context->formatter.tags[level - 1]);
    context->formatter.tags[level - 1] = tag_copy;
    return CLOG_SUCCESS;
}

static void clog_add_filter_to_chain(clog_filter_t *head, clog_filter_t *filter)
{
    clog_filter_t *last = head;
    const clog_filter_t *cur = head->next;

    while (cur != NULL) {
        if (filter->priority < cur->priority) {
            break;
        }
        last = (clog_filter_t *)cur;
        cur = cur->next;
    }

    last->next = filter;
    filter->next = cur;
}

clog_res_e clog_add_filter(clog_context_t *context, clog_filter_t *filter)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(filter, CLOG_INVALID_PARAM, "filter is NULL");
    CLOG_RET_IF_X(filter->type != CLOG_FILTER_PRE && filter->type != CLOG_FILTER_POST, CLOG_INVALID_PARAM,
                  "filter.type %u is invalid", filter->type);
    CLOG_RET_IF_NULL_X(filter->filter, CLOG_INVALID_PARAM, "filter.filter is NULL");
    CLOG_RET_IF_X(filter->next != NULL, CLOG_INVALID_PARAM, "filter.next should be NULL");
    clog_filter_t *filter_copy = clog_malloc(sizeof(clog_filter_t));
    CLOG_RET_IF_NULL_X(filter_copy, CLOG_NO_MEMORY, "clog_malloc clog_filter_t failed");
    *filter_copy = *filter;
    if (filter_copy->type == CLOG_FILTER_PRE) {
        clog_add_filter_to_chain(&context->filters.pre, filter);
    } else {
        clog_add_filter_to_chain(&context->filters.post, filter);
    }
    return CLOG_SUCCESS;
}

clog_res_e clog_set_channel(clog_context_t *context, clog_channel_t *channel)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(channel, CLOG_INVALID_PARAM, "channel is NULL");
    CLOG_RET_IF_NULL_X(channel->open, CLOG_INVALID_PARAM, "channel->open is NULL");
    CLOG_RET_IF_NULL_X(channel->write, CLOG_INVALID_PARAM, "channel->write is NULL");
    CLOG_RET_IF_NULL_X(channel->read, CLOG_INVALID_PARAM, "channel->read is NULL");
    CLOG_RET_IF_NULL_X(channel->close, CLOG_INVALID_PARAM, "channel->close is NULL");

    clog_channel_destroy(&context->channel);
    context->channel = channel;
    return CLOG_SUCCESS;
}

clog_res_e clog_set_dispatcher(clog_context_t *context, clog_dispatcher_t *dispatcher)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(dispatcher, CLOG_INVALID_PARAM, "dispatcher is NULL");
    CLOG_RET_IF_X(dispatcher->channel != NULL, CLOG_INVALID_PARAM, "dispatcher->channel should be NULL");
    CLOG_RET_IF_NULL_X(dispatcher->open, CLOG_INVALID_PARAM, "dispatcher->open is NULL");
    CLOG_RET_IF_NULL_X(dispatcher->notify, CLOG_INVALID_PARAM, "dispatcher->notify is NULL");
    CLOG_RET_IF_NULL_X(dispatcher->close, CLOG_INVALID_PARAM, "dispatcher->close is NULL");

    clog_dispatcher_destroy(&context->dispatcher);
    context->dispatcher = dispatcher;
    return CLOG_SUCCESS;
}

static void *clog_recoder_dup(const void *ptr)
{
    clog_recorder_t *tmp = clog_malloc(sizeof(clog_recorder_t));
    CLOG_RET_IF_NULL(tmp, NULL);
    const clog_recorder_t *src = ptr;
    tmp->id = src->id;
    tmp->open = src->open;
    tmp->write = src->write;
    tmp->flush = src->flush;
    tmp->close = src->close;
    tmp->extra = src->extra;
    return tmp;
}

static void clog_recoder_free(void *ptr)
{
    clog_recorder_t *tmp = ptr;
    tmp->close(tmp);
    clog_free(tmp);
}

clog_res_e clog_add_recorder(clog_context_t *context, const clog_recorder_t *recorder)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(recorder, CLOG_INVALID_PARAM, "recorder is NULL");
    CLOG_RET_IF_NULL_X(recorder->open, CLOG_INVALID_PARAM, "recorder->open is NULL");
    CLOG_RET_IF_NULL_X(recorder->write, CLOG_INVALID_PARAM, "recorder->write is NULL");
    CLOG_RET_IF_NULL_X(recorder->flush, CLOG_INVALID_PARAM, "recorder->flush is NULL");
    CLOG_RET_IF_NULL_X(recorder->close, CLOG_INVALID_PARAM, "recorder->close is NULL");

    if (context->recorders == NULL) {
        context->recorders =
            clog_hashmap_create(sizeof(uint32_t), 0, NULL, NULL, clog_recoder_dup, clog_recoder_free, NULL, NULL, 0);
        CLOG_RET_IF_NULL_X(context->recorders, CLOG_NO_MEMORY, "clog_hashmap_create recorders failed");
    }

    const clog_res_e ret = clog_hashmap_put(context->recorders, &recorder->id, recorder);
    CLOG_RET_IF_FAILED_X(ret, "clog_hashmap_put recorder failed, id = %u, ret = %u", recorder->id, ret);
    return CLOG_SUCCESS;
}

clog_res_e clog_context_setup(clog_context_t *context)
{
    CLOG_RET_IF_NULL_X(context, CLOG_INVALID_PARAM, "context is NULL");
    CLOG_CONTEXT_SETUP_STATE_CHECK()
    CLOG_RET_IF_NULL_X(context->modules, CLOG_NOT_COMPLETED, "modules is empty");
    CLOG_RET_IF_NULL_X(context->formatter.interpolator, CLOG_NOT_COMPLETED, "log format not configured");
    CLOG_RET_IF_NULL_X(context->channel, CLOG_NOT_COMPLETED, "channel not set");
    CLOG_RET_IF_NULL_X(context->dispatcher, CLOG_NOT_COMPLETED, "dispatcher not set");

    clog_res_e ret = context->channel->open(context->channel, NULL);
    CLOG_RET_IF_FUNC_FAILED_X(context->channel->open, ret);
    context->dispatcher->channel = context->channel;
    ret = context->dispatcher->open(context->dispatcher, NULL);
    CLOG_RET_IF_FUNC_FAILED_X(context->dispatcher->open, ret);
    context->dispatcher->notify(context->dispatcher, CLOG_DISPATCHER_EVENT_START);

    /* start all recorder */
    clog_hashmap_iterator_t *it = clog_hashmap_iterator_create(context->recorders);
    CLOG_RET_IF_NULL_X(it, CLOG_FAIL, "clog_hashmap_iterator_create failed");
    while (clog_hashmap_iterator_key(it) != NULL) {
        clog_recorder_t *recorder = clog_hashmap_iterator_value(it);
        CLOG_CLEAN_RET_IF_NULL_X(recorder, clog_hashmap_iterator_destroy(&it), CLOG_FAIL, "iterator value is NULL");
        ret = recorder->open(recorder, NULL);
        CLOG_CLEAN_RET_IF_FAILED_X(ret, clog_hashmap_iterator_destroy(&it), "recorder %u open failed, ret = %u",
                                   recorder->id, ret);
        if (!clog_hashmap_iterator_next(it)) {
            break;
        }
    }
    clog_hashmap_iterator_destroy(&it);
    clog_interpolator_context_destroy(&context->formatter.interpolator_context);
    clog_hashmap_destroy(&context->formatter.placeholders);
    context->state = CLOG_STATE_RUNNING;
    return CLOG_SUCCESS;
}

void clog_context_destroy(clog_context_t **context)
{
    CLOG_RET_VOID_IF_NULL_X(context, "context is NULL");
    CLOG_RET_VOID_IF_NULL_X(*context, "*context is NULL");
    clog_context_t *tmp = *context;
    clog_atomic_set(&tmp->state, CLOG_STATE_DESTROYED);
    /* dispatcher use channel and recorders, so close it first */
    if (tmp->dispatcher != NULL) {
        tmp->dispatcher->close(tmp->dispatcher);
        CLOG_SAFE_FREE(tmp->dispatcher);
    }
    if (tmp->channel != NULL) {
        tmp->channel->close(tmp->channel);
        CLOG_SAFE_FREE(tmp->channel);
    }
    clog_hashmap_destroy(&tmp->recorders);
    clog_interpolator_clear(&tmp->formatter.interpolator);
    if (tmp->formatter.interpolator_context != NULL) {
        clog_interpolator_context_destroy(&tmp->formatter.interpolator_context);
    }
    if (tmp->formatter.placeholders != NULL) {
        clog_hashmap_destroy(&tmp->formatter.placeholders);
    }
    for (size_t i = 0; i < CLOG_ARRAY_SIZE(tmp->formatter.tags); ++i) {
        CLOG_SAFE_FREE(tmp->formatter.tags[i]);
    }
    clog_filter_free((clog_filter_t *)tmp->filters.pre.next);
    tmp->filters.pre.next = NULL;
    clog_filter_free((clog_filter_t *)tmp->filters.post.next);
    tmp->filters.post.next = NULL;
    clog_hashmap_destroy(&tmp->modules);
#ifdef CASCA_LOG_MEM_POOL
    clog_buffer_pool_finalize(tmp->pool);
    tmp->pool = NULL;
#endif
    clog_free(tmp);
    *context = NULL;
}
