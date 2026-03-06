/**
 * @author Polaris
 * @date  2025/12/4
 */
#ifndef CASCA_LOG_CLOG_CHANNEL_BASE_H
#define CASCA_LOG_CLOG_CHANNEL_BASE_H

#include "casca_log_base.h"
#include "clog_config.h"
#include "clog_hooks.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_CHANNEL_ID_INVALID 0
#define CLOG_CHANNEL_ID_ATOMIC_QUEUE 1
#define CLOG_CHANNEL_ID_RESERVED 0xFFFFU

typedef struct clog_channel clog_channel_t;
typedef uint32_t clog_channel_id_t;
typedef void (*clog_channel_provider_f)(clog_channel_t *channel);

typedef clog_res_e (*clog_channel_open_f)(clog_channel_t *self, const clog_config_group_t *config);
typedef clog_res_e (*clog_channel_write_f)(clog_channel_t *self, const clog_item_t *item);
typedef clog_res_e (*clog_channel_read_f)(clog_channel_t *self, const clog_item_t **item);
typedef void (*clog_channel_close_f)(clog_channel_t *self);

struct clog_channel {
    clog_channel_id_t id;
    clog_channel_open_f open;
    clog_channel_write_f write;
    clog_channel_read_f read;
    clog_channel_close_f close;
    void *param;
};

/**
 * destroy channel
 * @param channel channel
 */
static void clog_channel_destroy(clog_channel_t **channel)
{
    if (channel != NULL) {
        if (*channel != NULL) {
            (*channel)->close(*channel);
            clog_free(*channel);
            *channel = NULL;
        }
    }
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CHANNEL_BASE_H */
