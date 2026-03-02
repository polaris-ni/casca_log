/**
 * @author Polaris
 * @date  2025/12/6
 */
#ifndef CASCA_LOG_CLOG_CHANNEL_MANAGER_H
#define CASCA_LOG_CLOG_CHANNEL_MANAGER_H

#include "clog_channel_base.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * register customize channel provider
 * @param id channel id, should be greater than CLOG_CHANNEL_ID_RESERVED
 * @param provider customized channel
 * @return #CLOG_SUCCESS if success
 *          #CLOG_NO_MEMORY if malloc failed
 *          #CLOG_ALREADY_EXISTED if channel already registered
 *          #CLOG_INVALID_PARAM if id is less than CLOG_CHANNEL_ID_RESERVED
 */
clog_res_e clog_channel_register_provider(clog_channel_id_t id, clog_channel_provider_f provider);

/**
 * get channel by id
 * @param id channel id
 * @param channel channel
 * @return #CLOG_SUCCESS if success, #CLOG_TARGET_NOT_FOUND if channel not found, #CLOG_INVALID_PARAM if channel is NULL
 */
clog_res_e clog_channel_get(clog_channel_id_t id, clog_channel_t *channel);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_CHANNEL_MANAGER_H */
