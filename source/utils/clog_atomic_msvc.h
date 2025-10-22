/**
 * @author Polaris
 * @date  2025/10/22
 */
#ifndef CASCA_LOG_CLOG_ATOMIC_MSVC_H
#define CASCA_LOG_CLOG_ATOMIC_MSVC_H

#include <intrin.h>
#include <windows.h>
#include "clog_platform.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* Fuck MSVC */
typedef enum {
    atomic_memory_order_relaxed,
    atomic_memory_order_acquire,
    atomic_memory_order_release,
    atomic_memory_order_acq_rel,
    atomic_memory_order_seq_cst
} atomic_memory_order_t;

#ifdef CLOG_ARCH_64BIT
typedef LONGLONG clog_atomic_basic_t;
typedef volatile clog_atomic_basic_t atomic_intptr_t;
typedef volatile clog_atomic_basic_t atomic_uintptr_t;
#define atomic_store(object, desired) InterlockedExchange64(object, desired)
#define atomic_exchange(object, desired) InterlockedExchange64(object, desired)
#define atomic_fetch_add(object, operand) InterlockedExchangeAdd64(object, operand)
#define atomic_fetch_sub(object, operand) InterlockedExchangeAdd64(object, -operand)
#else
typedef LONG clog_atomic_basic_t;
typedef volatile clog_atomic_basic_t atomic_intptr_t;
typedef volatile clog_atomic_basic_t atomic_uintptr_t;
#define atomic_store(object, desired) InterlockedExchange(object, desired)
#define atomic_exchange(object, desired) InterlockedExchange(object, desired)
#define atomic_fetch_add(object, operand) InterlockedExchangeAdd(object, operand)
#define atomic_fetch_sub(object, operand) InterlockedExchangeAdd(object, -operand)
#endif

#define atomic_store_explicit(object, desired, order) atomic_store(object, desired)
#define atomic_exchange_explicit(object, desired, order) atomic_exchange(object, desired)
#define atomic_fetch_add_explicit(object, operand, order) atomic_fetch_add(object, operand)
#define atomic_fetch_sub_explicit(object, operand, order) atomic_fetch_sub(object, operand)

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

static inline void atomic_fence(const atomic_memory_order_t mo)
{
    _ReadWriteBarrier();
#if defined(_M_ARM) || defined(_M_ARM64)
    /* ARM needs a barrier for everything but relaxed. */
    if (mo != atomic_memory_order_relaxed) {
        MemoryBarrier();
    }
#elif defined(_M_IX86) || defined(_M_X64)
    /* x86 needs a barrier only for seq_cst. */
    if (mo == atomic_memory_order_seq_cst) {
        MemoryBarrier();
    }
#else
#error "platform not support"
#endif
    _ReadWriteBarrier();
}

static inline int atomic_compare_exchange_strong(volatile clog_atomic_basic_t* object, clog_atomic_basic_t* expected,
                                                 clog_atomic_basic_t desired)
{
#ifdef CLOG_ARCH_64BIT
    const clog_atomic_basic_t old = InterlockedCompareExchange64(object, desired, *expected);
#else
    const clog_atomic_basic_t old = InterlockedCompareExchange(object, desired, *expected);
#endif
    if (old != *expected) {
        *expected = old;
        return 0;
    }
    return 1;
}

#define atomic_compare_exchange_strong_explicit(object, expected, desired, success, fail) \
    atomic_compare_exchange_strong(object, expected, desired)

#define atomic_compare_exchange_weak(object, expected, desired) \
    atomic_compare_exchange_strong(object, expected, desired)

#define atomic_compare_exchange_weak_explicit(object, expected, desired, success, fail) \
    atomic_compare_exchange_strong_explicit(object, expected, desired, success, fail)


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_ATOMIC_MSVC_H */
