/**
 * @auther Polaris
 * @date  2025/9/10
 */
#ifndef CASCA_LOG_CLOG_ATOMIC_H
#define CASCA_LOG_CLOG_ATOMIC_H

#include "clog_platform.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef CLOG_PLATFORM_WINDOWS
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#include <stdatomic.h> /* supported std atomic */
#else
/* std atomic compat start */
#include <stdint.h>
#include <windows.h>

/* 确保包含Intrinsics函数 */
#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_InterlockedExchange16)
#pragma intrinsic(_InterlockedCompareExchange16)
#endif

/* atomic types */
typedef volatile int atomic_int;
typedef volatile unsigned int atomic_uint;
typedef volatile long atomic_long;
typedef volatile unsigned long atomic_ulong;
typedef volatile long long atomic_llong;
typedef volatile unsigned long long atomic_ullong;
typedef volatile ptrdiff_t atomic_ptrdiff_t;
typedef volatile size_t atomic_size_t;
typedef volatile void* atomic_void_ptr;
typedef volatile intptr_t atomic_intptr_t;
typedef volatile uintptr_t atomic_uintptr_t;
typedef volatile int16_t atomic_int16_t;
typedef volatile uint16_t atomic_uint16_t;
typedef volatile int32_t atomic_int32_t;
typedef volatile uint32_t atomic_uint32_t;
typedef volatile int64_t atomic_int64_t;
typedef volatile uint64_t atomic_uint64_t;

/* memory order */
typedef enum {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
} memory_order;

/* memory operation */
#define atomic_init(obj, value) (*(obj) = (value))
#define atomic_store(obj, desired) InterlockedExchange((LONG volatile*)(obj), (LONG)(desired))
#define atomic_load(obj) InterlockedCompareExchange((LONG volatile*)(obj), 0, 0)
#define atomic_exchange(obj, desired) InterlockedExchange((LONG volatile*)(obj), (LONG)(desired))
#define atomic_fetch_add(obj, arg) InterlockedExchangeAdd((LONG volatile*)(obj), (LONG)(arg))
#define atomic_fetch_sub(obj, arg) InterlockedExchangeAdd((LONG volatile*)(obj), -(LONG)(arg))
#define atomic_fetch_or(obj, arg) InterlockedOr((LONG volatile*)(obj), (LONG)(arg))
#define atomic_fetch_and(obj, arg) InterlockedAnd((LONG volatile*)(obj), (LONG)(arg))
#define atomic_fetch_xor(obj, arg) InterlockedXor((LONG volatile*)(obj), (LONG)(arg))

static inline int atomic_compare_exchange_strong(volatile void* obj, void* expected, long int desired)
{
    long int expected_val = *(long int*)expected;
    long int old_val = InterlockedCompareExchange((LONG volatile*)obj, desired, expected_val);
    if (old_val == expected_val) {
        return 1;
    }
    else {
        *(long int*)expected = old_val;
        return 0;
    }
}

static inline int atomic_compare_exchange_weak(volatile void* obj, void* expected, long int desired)
{
    return atomic_compare_exchange_strong(obj, expected, desired);
}

/* memory barrier */
#define atomic_thread_fence(order) MemoryBarrier()
#define atomic_signal_fence(order) _ReadWriteBarrier()

/* std atomic compat end */
#endif
#elif
#include <stdatomic.h>
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_ATOMIC_H */
