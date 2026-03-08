/**
 * @author Polaris
 * @date 2026/2/23
 */

#ifndef CASCA_LOG_CLOG_INTERPOLATOR_H
#define CASCA_LOG_CLOG_INTERPOLATOR_H

#include "clog_hashmap.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define CLOG_INTERPOLATOR_PLACEHOLDER_NAME_MAX_SIZE 32U

typedef struct clog_interpolator_context clog_interpolator_context_t;

/**
 * interpolator placeholder handler
 * @param param receive extra param
 * @param buf result string buffer
 * @param size buffer size
 * @return return interpolated string length('\0' is not include) if success, otherwise return -clog_res_e
 */
typedef int (*clog_placeholder_handler_f)(void *param, char *buf, size_t size);

typedef struct clog_interpolator clog_interpolator_t;

/**
 * create a new interpolator context
 * @param map placeholder handler hashmap
 * @return #clog_interpolator_context_t
 */
clog_interpolator_context_t *clog_interpolator_context_create(clog_hashmap_t *map);

/**
 * register placeholder
 * @param context #clog_interpolator_context_t
 * @param name placeholder name
 * @param handler placeholder handler
 * @return #clog_res_e
 */
clog_res_e clog_interpolator_context_register(const clog_interpolator_context_t *context, const char *name,
                                              clog_placeholder_handler_f handler);

/**
 * destroy interpolator context, *context will be set to NULL
 * @param context #clog_interpolator_context_t
 */
void clog_interpolator_context_destroy(clog_interpolator_context_t **context);

/**
 * parse interpolator string
 * @param context #clog_interpolator_context_t
 * @param fmt interpolator string
 * @param interpolator parsed interpolator
 * @return #clog_res_e
 */
clog_res_e clog_interpolator_parse(const clog_interpolator_context_t *context, const char *fmt,
                                   clog_interpolator_t **interpolator);

/**
 * interpolate interpolator string
 * @param interpolator parsed interpolator
 * @param param extra param, will be passed to placeholder handler
 * @param buf result string buffer
 * @param size buffer size('\0' included)
 * @param num if not NULL, it will be ste result string character num('\0' not included)
 * @return #clog_res_e
 */
clog_res_e clog_interpolator_interpolate(const clog_interpolator_t *interpolator, void *param, char *buf, size_t size,
                                         size_t *num);

/**
 * clear parsed interpolator
 * @param interpolator interpolator, it will be free
 */
void clog_interpolator_clear(clog_interpolator_t **interpolator);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_INTERPOLATOR_H */
