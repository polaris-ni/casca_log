/**
 * @author Polaris
 * @date  2025/11/1
 */
#ifndef CASCA_LOG_CLOG_ATOMIC_TYPES_H
#define CASCA_LOG_CLOG_ATOMIC_TYPES_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef atomic_uintptr_t clog_atomic_type_t;
typedef uintptr_t clog_atomic_basic_t;

static inline void clog_atomic_set(clog_atomic_type_t *obj, clog_atomic_basic_t val)
{
    atomic_store(obj, val);
}

static inline clog_atomic_basic_t clog_atomic_get(const clog_atomic_type_t *obj)
{
    return atomic_load(obj);
}

static inline bool clog_atomic_cas(clog_atomic_type_t *obj, clog_atomic_basic_t *expected, clog_atomic_basic_t desired)
{
    return atomic_compare_exchange_strong(obj, expected, desired);
}

static inline clog_atomic_basic_t clog_atomic_fetch_add(clog_atomic_type_t *obj, const clog_atomic_basic_t value)
{
    return atomic_fetch_add(obj, value);
}

static inline clog_atomic_basic_t clog_atomic_fetch_sub(clog_atomic_type_t *obj, const clog_atomic_basic_t value)
{
    return atomic_fetch_sub(obj, value);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_ATOMIC_TYPES_H */
