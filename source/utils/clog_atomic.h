/**
 * @author Polaris
 * @date  2025/9/10
 */
#ifndef CASCA_LOG_CLOG_ATOMIC_H
#define CASCA_LOG_CLOG_ATOMIC_H

#include "clog_platform.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef __has_include
#if __has_include(<stdatomic.h>) && !defined(_MSC_VER)
#include <stdatomic.h>
#define CLOG_USE_NATIVE_STD_ATOMIC
#endif
#else
#ifdef CLOG_PLATFORM_WINDOWS
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#include <stdatomic.h>
#define CLOG_USE_NATIVE_STD_ATOMIC
#endif
#endif
#endif

#ifndef CLOG_USE_NATIVE_STD_ATOMIC

#include <intrin.h>
#include <stdint.h>
#include <windows.h>

typedef enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
} memory_order;

#define ATOMIC_VAR_INIT(value) (value)

typedef volatile LONG atomic_flag;
typedef volatile LONG atomic_bool;
typedef volatile LONG atomic_char;
typedef volatile LONG atomic_schar;
typedef volatile ULONG atomic_uchar;
typedef volatile SHORT atomic_short;
typedef volatile USHORT atomic_ushort;
typedef volatile LONG atomic_int;
typedef volatile ULONG atomic_uint;
typedef volatile LONG atomic_long;
typedef volatile ULONG atomic_ulong;
typedef volatile LONGLONG atomic_llong;
typedef volatile ULONGLONG atomic_ullong;

#ifdef _WIN64
typedef volatile LONGLONG atomic_intptr_t;
typedef volatile LONGLONG atomic_uintptr_t;
typedef volatile LONGLONG atomic_size_t;
typedef volatile LONGLONG atomic_ptrdiff_t;
#else
typedef volatile LONG atomic_intptr_t;
typedef volatile LONG atomic_uintptr_t;
typedef volatile LONG atomic_size_t;
typedef volatile LONG atomic_ptrdiff_t;
#endif

#define atomic_load(object) (*(object))
#define atomic_load_explicit(object, order) atomic_load(object)
#define atomic_store(object, desired) InterlockedExchange((LONG volatile*)(object), (LONG)(desired))
#define atomic_store_explicit(object, desired, order) atomic_store(object, desired)
#define atomic_exchange(object, desired) InterlockedExchange((LONG volatile*)(object), (LONG)(desired))
#define atomic_exchange_explicit(object, desired, order) atomic_exchange(object, desired)
#define atomic_compare_exchange_weak(object, expected, desired) \
    atomic_compare_exchange_strong(object, expected, desired)

#define atomic_compare_exchange_weak_explicit(object, expected, desired, succ, fail) \
    atomic_compare_exchange_strong_explicit(object, expected, desired, succ, fail)

static inline int atomic_compare_exchange_strong(volatile LONG* object, LONG* expected, LONG desired)
{
    LONG old = InterlockedCompareExchange((LONG volatile*)object, desired, *expected);
    if (old == *expected) {
        return 1;
    }
    else {
        *expected = old;
        return 0;
    }
}

#define atomic_compare_exchange_strong_explicit(object, expected, desired, succ, fail) \
    atomic_compare_exchange_strong(object, expected, desired)
#define atomic_fetch_add(object, operand) InterlockedExchangeAdd((LONG volatile*)(object), (LONG)(operand))
#define atomic_fetch_add_explicit(object, operand, order) atomic_fetch_add(object, operand)
#define atomic_fetch_sub(object, operand) InterlockedExchangeAdd((LONG volatile*)(object), -(LONG)(operand))
#define atomic_fetch_sub_explicit(object, operand, order) atomic_fetch_sub(object, operand)
#define atomic_fetch_and(object, operand) InterlockedAnd((LONG volatile*)(object), (LONG)(operand))
#define atomic_fetch_and_explicit(object, operand, order) atomic_fetch_and(object, operand)
#define atomic_fetch_or(object, operand) InterlockedOr((LONG volatile*)(object), (LONG)(operand))
#define atomic_fetch_or_explicit(object, operand, order) atomic_fetch_or(object, operand)
#define atomic_fetch_xor(object, operand) InterlockedXor((LONG volatile*)(object), (LONG)(operand))
#define atomic_fetch_xor_explicit(object, operand, order) atomic_fetch_xor(object, operand)
#define atomic_flag_test_and_set(object) (InterlockedExchange((LONG volatile*)(object), 1) != 0)
#define atomic_flag_test_and_set_explicit(object, order) atomic_flag_test_and_set(object)
#define atomic_flag_clear(object) InterlockedExchange((LONG volatile*)(object), 0)
#define atomic_flag_clear_explicit(object, order) atomic_flag_clear(object)
#define atomic_thread_fence(order) MemoryBarrier()
#define atomic_signal_fence(order) ((void)0)
#define atomic_init(object, value) \
    do {                           \
        *(object) = (value);       \
    } while (0)

#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_ATOMIC_H */
