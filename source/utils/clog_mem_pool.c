/**
 * @auther Polaris
 * @date  2025/9/9
 */
#include "clog_mem_pool.h"
#include <stdint.h>
#include "casca_log.h"
#include "casca_log_defines.h"

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

typedef struct mem_block mem_block_t;

struct mem_block {
    struct {
        uint32_t type : 4; /* #CLOG_MEM_BLOCK_HEAP or #CLOG_MEM_BLOCK_TEMP */
        uint32_t size : 4; /* refer to #clog_heap_type_e */
        uint32_t reserved : 24; /* reserved */
    };
    mem_block_t* next; /* next #mem_block_t */
    char data[0]; /* memory for use */
};

#define CLOG_MP_NOT_INIT 0
#define CLOG_MP_RUNNING 1
#define CLOG_MP_STOP 2

typedef struct mem_heap {
    clog_heap_type_e type;
    uint32_t flag; /* #CLOG_MP_NOT_INIT #CLOG_MP_RUNNING #CLOG_MP_STOP */
    uint32_t used; /* the number of memory that allocated from #mem_heap_t */
    uint32_t allocated; /* the number of memory that allocated from system */
    mem_block_t* head;
    mem_block_t* tail;
} mem_heap_t;

static mem_heap_t g_mem_heap[CLOG_MP_HEAP_TYPE_MAX] = {0};

static mem_block_t* clog_mp_malloc_block(const size_t size)
{
    mem_block_t* block = clog_malloc(sizeof(mem_block_t) + size);
    CLOG_RET_IF_NULL(block, NULL);
    block->next = NULL;
    block->reserved = 0;
    block->next = NULL;
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

static void clog_mp_pre_allocated(mem_heap_t* heap, size_t num)
{
    for (size_t i = 0; i < num; i++) {
        mem_block_t* block = clog_malloc_heap_block(heap->type);
        CLOG_RET_VOID_IF_NULL(block);
        if (heap->head == NULL) {
            heap->head = block;
            heap->tail = block;
        }
        else {
            heap->tail->next = block;
            heap->tail = block;
        }
        heap->allocated++;
    }
}

void clog_mp_init(const size_t level)
{
    const size_t cnt = CLOG_ARRAY_SIZE(g_mem_heap);
    for (size_t i = 0; i < cnt; i++) {
        mem_heap_t* heap = &g_mem_heap[i];
        heap->type = i;
        heap->used = 0;
        heap->allocated = 0;
        heap->head = NULL;
        heap->tail = NULL;
        heap->flag = CLOG_MP_RUNNING;
    }
    if (level >= CLOG_MP_PRE_ALLOCATED_LESS) {
        clog_mp_pre_allocated(&g_mem_heap[CLOG_MP_HEAP_TYPE_8], 16);
        clog_mp_pre_allocated(&g_mem_heap[CLOG_MP_HEAP_TYPE_16], 16);
        clog_mp_pre_allocated(&g_mem_heap[CLOG_MP_HEAP_TYPE_32], 16);
    }
    if (level >= CLOG_MP_PRE_ALLOCATED_NORMAL) {
        clog_mp_pre_allocated(&g_mem_heap[CLOG_MP_HEAP_TYPE_64], 8);
        clog_mp_pre_allocated(&g_mem_heap[CLOG_MP_HEAP_TYPE_128], 8);
        clog_mp_pre_allocated(&g_mem_heap[CLOG_MP_HEAP_TYPE_256], 8);
    }
    if (level >= CLOG_MP_PRE_ALLOCATED_FULL) {
        clog_mp_pre_allocated(&g_mem_heap[CLOG_MP_HEAP_TYPE_512], 4);
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

void* clog_mp_allocate(const size_t size)
{
    CLOG_RET_IF(size == 0, NULL);
    mem_heap_t* heap = clog_get_mem_heap(size);
    if (heap == NULL) {
        return clog_malloc_temp_block(size);
    }

    if (heap->head == NULL) {
        mem_block_t* block = clog_malloc_heap_block(heap->type);
        CLOG_RET_IF_NULL(block, NULL);
        heap->allocated++;
        heap->used++;
        return block->data;
    }

    heap->used++;
    mem_block_t* block = heap->head;
    heap->head = block->next;
    if (heap->head == NULL) {
        heap->tail = NULL;
    }
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
    mem_heap_t* heap = &g_mem_heap[block->type];
    if (heap->flag != CLOG_MP_RUNNING) {
        if (heap->flag == CLOG_MP_STOP) {
            heap->used--;
        }
        clog_free(block);
        return;
    }
    if (heap->tail == NULL) {
        heap->head = block;
        heap->tail = block;
        heap->used--;
        return;
    }
    heap->tail->next = block;
    heap->tail = block;
    heap->used--;
}

static void clog_mp_clear(mem_heap_t* heap)
{
    mem_block_t* block = heap->head;
    mem_block_t* next = NULL;
    while (block != NULL) {
        next = block->next;
        clog_free(block);
        block = next;
        heap->allocated--;
    }
    heap->flag = CLOG_MP_STOP;
}

void clog_mp_finalize(void)
{
    for (size_t i = 0; i < CLOG_MP_HEAP_TYPE_MAX; i++) {
        clog_mp_clear(&g_mem_heap[i]);
    }
}
