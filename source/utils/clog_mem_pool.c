/**
 * @auther Polaris
 * @date  2025/9/9
 */
#include "clog_mem_pool.h"
#include <stdint.h>
#include "casca_log.h"
#include "casca_log_defines.h"
#include "clog_atomic.h"

#define CLOG_MP_HEAP_SIZE_8 8
#define CLOG_MP_HEAP_SIZE_16 16
#define CLOG_MP_HEAP_SIZE_32 32
#define CLOG_MP_HEAP_SIZE_64 64
#define CLOG_MP_HEAP_SIZE_128 128
#define CLOG_MP_HEAP_SIZE_256 256
#define CLOG_MP_HEAP_SIZE_512 512

typedef enum clog_heap_type {
    CLOG_MP_HEAP_TYPE_8 = 0,
    CLOG_MP_HEAP_TYPE_16,
    CLOG_MP_HEAP_TYPE_32,
    CLOG_MP_HEAP_TYPE_64,
    CLOG_MP_HEAP_TYPE_128,
    CLOG_MP_HEAP_TYPE_256,
    CLOG_MP_HEAP_TYPE_512,
    CLOG_MP_HEAP_TYPE_MAX
} clog_heap_type_e;

#define CLOG_GET_HEAP_SIZE(type) (1 << ((type) + 3))

#define CLOG_MEM_BLOCK_HEAP 1 /* heap block, will put back to heap when clog_mp_release  */
#define CLOG_MEM_BLOCK_TEMP 2 /* temp block, will be free when clog_mp_release  */

#define CLOG_U_PTR_NULL ((uintptr_t)0)

typedef struct mem_block mem_block_t;

struct mem_block {
    struct {
        uint32_t type : 4; /* #CLOG_MEM_BLOCK_HEAP or #CLOG_MEM_BLOCK_TEMP */
        uint32_t size : 4; /* refer to #clog_heap_type_e */
        uint32_t reserved : 24; /* reserved */
    };
    atomic_uintptr_t next; /* next #mem_block_t */
    char data[0]; /* memory for use */
};

typedef struct mem_heap {
    clog_heap_type_e type;
    atomic_uint allocated;
    atomic_uint total;
    atomic_uintptr_t head;
    atomic_uintptr_t tail;
} mem_heap_t;

static mem_heap_t g_mem_heap[CLOG_MP_HEAP_TYPE_MAX] = {0};

void clog_mp_init(void)
{
    const size_t cnt = CLOG_ARRAY_SIZE(g_mem_heap);
    for (size_t i = 0; i < cnt; i++) {
        mem_heap_t* heap = &g_mem_heap[i];
        heap->type = i;
        atomic_init(&heap->allocated, 0);
        atomic_init(&heap->total, 0);
        atomic_init(&heap->head, CLOG_U_PTR_NULL);
        atomic_init(&heap->tail, CLOG_U_PTR_NULL);
    }
}

static mem_heap_t* clog_get_mem_heap(const size_t size)
{
    for (size_t i = 0; i < CLOG_MP_HEAP_TYPE_MAX; i++) {
        if (size <= CLOG_GET_HEAP_SIZE(i)) {
            return &g_mem_heap[i];
        }
    }
    return NULL;
}

static mem_block_t* clog_mp_malloc_block(const size_t size)
{
    mem_block_t* block = clog_malloc(sizeof(mem_block_t) + size);
    CLOG_RET_IF_NULL(block, NULL);
    atomic_store(&block->next, (uintptr_t)0);
    block->reserved = 0;
    return block;
}

static void* clog_malloc_temp_block(const size_t size)
{
    mem_block_t* block = clog_mp_malloc_block(size);
    CLOG_RET_IF_NULL(block, NULL);
    block->type = CLOG_MEM_BLOCK_TEMP;
    block->size = CLOG_MP_HEAP_TYPE_MAX;
    return block->data;
}

static mem_block_t* clog_malloc_heap_block(const clog_heap_type_e type)
{
    mem_block_t* block = clog_mp_malloc_block(CLOG_GET_HEAP_SIZE(type));
    CLOG_RET_IF_NULL(block, NULL);
    block->type = CLOG_MEM_BLOCK_HEAP;
    block->size = type;
    return block;
}

void* clog_mp_allocate(const size_t size)
{
    CLOG_RET_IF(size == 0, NULL);
    mem_heap_t* heap = clog_get_mem_heap(size);
    if (heap == NULL) {
        return clog_malloc_temp_block(size);
    }

    mem_block_t* block = NULL;

    // First try to get block from heap
    do {
        block = (mem_block_t*)atomic_load(&heap->head);
        if (block == NULL) {
            // No available block, allocate new one
            atomic_fetch_add(&heap->allocated, 1);
            atomic_fetch_add(&heap->total, 1);
            return clog_malloc_heap_block(heap->type)->data;
        }
    } while (!atomic_compare_exchange_weak(&heap->head, (uintptr_t*)&block, atomic_load(&block->next)));

    // Successfully obtained a block from heap
    CLOG_RET_IF_NULL(block, NULL);
    atomic_store(&block->next, CLOG_U_PTR_NULL);
    return block->data;
}

void clog_mp_release(void* ptr)
{
    CLOG_RET_VOID_IF_NULL(ptr);
    mem_block_t* block = (mem_block_t*)((uintptr_t)ptr - offsetof(mem_block_t, data));
    if (block->type == CLOG_MEM_BLOCK_TEMP) {
        clog_free(block);
        return;
    }
    /* TODO: put back to heap */
}
