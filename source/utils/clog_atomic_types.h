/**
 * @author Polaris
 * @date  2025/11/1
 */
#ifndef CASCA_LOG_CLOG_ATOMIC_TYPES_H
#define CASCA_LOG_CLOG_ATOMIC_TYPES_H

#include <stdbool.h>
#include "clog_platform.h"
#ifdef CLOG_COMPILER_MSVC
#include <Windows.h>
#include <intrin.h>
#else
#include <stdatomic.h>
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* Fuck MSVC */
#ifdef CLOG_COMPILER_MSVC
#ifdef CLOG_ARCH_64BIT
typedef LONGLONG clog_atomic_basic_t;
#else
typedef LONG clog_atomic_basic_t;
#endif
typedef volatile clog_atomic_basic_t clog_atomic_type_t;
#else
typedef atomic_uintptr_t clog_atomic_type_t;
typedef uintptr_t clog_atomic_basic_t;
#endif

#ifdef CLOG_COMPILER_MSVC
typedef enum {
    atomic_memory_order_relaxed,
    atomic_memory_order_acquire,
    atomic_memory_order_release,
    atomic_memory_order_acq_rel,
    atomic_memory_order_seq_cst
} atomic_memory_order_t;

static inline bool atomic_compare_exchange_strong(volatile clog_atomic_basic_t* object, clog_atomic_basic_t* expected,
                                                  clog_atomic_basic_t desired)
{
#ifdef CLOG_ARCH_64BIT
    const clog_atomic_basic_t old = InterlockedCompareExchange64(object, desired, *expected);
#else
    const clog_atomic_basic_t old = InterlockedCompareExchange(object, desired, *expected);
#endif
    if (old != *expected) {
        *expected = old;
        return false;
    }
    return true;
}

static inline clog_atomic_basic_t atomic_load_explicit(volatile const clog_atomic_basic_t* obj,
                                                       const atomic_memory_order_t order)
{
    clog_atomic_basic_t val;
    switch (order) {
        case atomic_memory_order_relaxed:
            val = *obj;
            break;
        case atomic_memory_order_acquire:
        case atomic_memory_order_seq_cst:
        default:
            _ReadWriteBarrier();
            val = *obj;
            _ReadWriteBarrier();
            break;
    }
    return val;
}

#define atomic_load(object) atomic_load_explicit(object, atomic_memory_order_seq_cst)

#ifdef CLOG_ARCH_64BIT
#define atomic_store(object, desired) InterlockedExchange64(object, desired)
#define atomic_exchange(object, desired) InterlockedExchange64(object, desired)
#define atomic_fetch_add(object, operand) InterlockedExchangeAdd64(object, operand)
#define atomic_fetch_sub(object, operand) InterlockedExchangeAdd64(object, -operand)
#else
#define atomic_store(object, desired) InterlockedExchange(object, desired)
#define atomic_exchange(object, desired) InterlockedExchange(object, desired)
#define atomic_fetch_add(object, operand) InterlockedExchangeAdd(object, operand)
#define atomic_fetch_sub(object, operand) InterlockedExchangeAdd(object, -operand)
#endif
#endif

static inline void clog_atomic_set(clog_atomic_type_t* obj, clog_atomic_basic_t val)
{
    atomic_store(obj, val);
}

static inline clog_atomic_basic_t clog_atomic_get(const clog_atomic_type_t* obj)
{
    return atomic_load(obj);
}

static inline bool clog_atomic_cas(clog_atomic_type_t* obj, clog_atomic_basic_t* expected, clog_atomic_basic_t desired)
{
    return atomic_compare_exchange_strong(obj, expected, desired);
}

static inline clog_atomic_basic_t clog_atomic_fetch_add(clog_atomic_type_t* obj, const clog_atomic_basic_t value)
{
#ifdef CLOG_COMPILER_MSVC
    return atomic_fetch_add(obj, value);
#else
    const clog_atomic_type_t tmp = atomic_fetch_add(obj, value);
    return tmp;
#endif
}

static inline clog_atomic_basic_t clog_atomic_fetch_sub(clog_atomic_type_t* obj, const clog_atomic_basic_t value)
{
#ifdef CLOG_COMPILER_MSVC
    return atomic_fetch_add(obj, value);
#else
    const clog_atomic_type_t tmp = atomic_fetch_add(obj, -value);
    return tmp;
#endif
}


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_ATOMIC_TYPES_H */
