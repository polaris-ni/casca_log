/**
 * @auther Polaris
 * @date  2025/10/2
 */
#include "clog_hashmap.h"
#include <stdlib.h>
#include <time.h>

#ifdef CLOG_ARCH_64BIT
typedef uint64_t clog_hashmap_hash_t;
#else
typedef uint32_t clog_hashmap_hash_t;
#endif

typedef clog_hashmap_hash_t (*clog_hashmap_hash_f)(const clog_hashmap_t* map, const void* key);

struct clog_hashmap {
    size_t key_size; /* if size of key is fixed, #key_size indicates the exact size, will be set to 0 otherwise */
    size_t value_size; /* if size of value is fixed, #value_size indicates the exact size, will be set to 0 otherwise */
    size_t data_size; /* the num of key-value */
    size_t buckets_size; /* the num of buckets */
    uint32_t seed; /* seed used in hash func */
    clog_hashmap_entry_t* buckets; /* buckets, first key-value entry will be stored in buckets[i].next  */
    clog_hashmap_hash_f hash; /* hash func */
    clog_hashmap_cmp_f cmp; /* key compare func */
    clog_hashmap_dup_f key_dup; /* key duplicated func */
    clog_hashmap_free_f key_free; /* key free func */
    clog_hashmap_dup_f value_dup; /* value duplicated func */
    clog_hashmap_free_f value_free; /* value free func */
    clog_hashmap_size_f size_of; /* calculate the size of key, if #size_of is NULL, #key_size will be used instead */
};

static void* clog_hashmap_dup_key(const clog_hashmap_t* map, const void* key)
{
    if (map->key_dup != NULL) {
        return map->key_dup(key);
    }
    if (map->key_size == 0) {
        return NULL;
    }
    void* tmp = clog_malloc(map->key_size);
    CLOG_RET_IF_NULL(tmp, NULL);
    const clog_res_e ret = clog_memcpy(tmp, map->key_size, key, map->key_size);
    if (ret != CLOG_SUCCESS) {
        clog_free(tmp);
        return NULL;
    }
    return tmp;
}

static void* clog_hashmap_dup_value(const clog_hashmap_t* map, const void* value)
{
    if (map->value_dup != NULL) {
        return map->value_dup(value);
    }
    if (map->value_size == 0) {
        return NULL;
    }
    void* tmp = clog_malloc(map->value_size);
    CLOG_RET_IF_NULL(tmp, NULL);
    const clog_res_e ret = clog_memcpy(tmp, map->value_size, value, map->value_size);
    if (ret != CLOG_SUCCESS) {
        clog_free(tmp);
        return NULL;
    }
    return tmp;
}

static void clog_hashmap_free_key(const clog_hashmap_t* map, void* key)
{
    if (map->key_free != NULL) {
        map->key_free(key);
    } else {
        clog_free(key);
    }
}

static void clog_hashmap_free_value(const clog_hashmap_t* map, void* value)
{
    if (map->value_free != NULL) {
        map->value_free(value);
    } else {
        clog_free(value);
    }
}

static bool clog_hashmap_cmp(const clog_hashmap_t* map, const void* key1, const void* key2)
{
    if (map->cmp != NULL) {
        return map->cmp(key1, key2);
    }

    if (map->key_size == 0) {
        return false;
    }

    return memcmp(key1, key2, map->key_size) == 0;
}

static size_t clog_hashmap_size_of_key(const clog_hashmap_t* map, const void* key)
{
    if (map->size_of != NULL) {
        return map->size_of(key);
    }

    return map->key_size;
}

static uint32_t clog_hashmap_get_index(const clog_hashmap_t* map, const void* key)
{
    return map->hash(map, key) & (map->buckets_size - 1);
}

static clog_hashmap_hash_t murmur3_32(const clog_hashmap_t* map, const void* key)
{
    const uint8_t* data = key;
    const size_t len = clog_hashmap_size_of_key(map, key);
    const size_t num = len / 4;
    uint32_t h1 = map->seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t* blocks = (const uint32_t*)(data + num * 4);
    for (size_t i = 0; i < num; ++i) {
        uint32_t k1 = blocks[-(int)i - 1]; /* little-endian read */
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;
        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64;
    }

    const uint8_t* tail = data + num * 4;
    uint32_t k1 = 0;
    switch (len & 3) {
        case 3:
            k1 ^= tail[2] << 16; /* fallthrough */
        case 2:
            k1 ^= tail[1] << 8; /* fallthrough */
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> 17);
            k1 *= c2;
            h1 ^= k1;
        default:
            /* do nothing */
            break;
    }

    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    return h1;
}

static clog_hashmap_entry_t* clog_hashmap_create_empty_buckets(const size_t size)
{
    const size_t len = size * sizeof(clog_hashmap_entry_t);
    clog_hashmap_entry_t* res = clog_malloc(len);
    CLOG_RET_IF_NULL(res, NULL);
    CLOG_IGNORE_RES(clog_memset(res, len, 0, len));
    return res;
}

clog_hashmap_t* clog_hashmap_create(const size_t key_size, const size_t value_size, const clog_hashmap_dup_f key_dup,
                                    const clog_hashmap_free_f key_free, const clog_hashmap_dup_f value_dup,
                                    const clog_hashmap_free_f value_free, const clog_hashmap_cmp_f cmp,
                                    const clog_hashmap_size_f size_of_key, uint32_t seed)
{
    clog_hashmap_t* map = clog_malloc(sizeof(clog_hashmap_t));
    CLOG_RET_IF_NULL(map, NULL);
    map->key_size = key_size;
    map->value_size = value_size;
    map->data_size = 0;
    map->buckets_size = 16;
    if (seed == 0) {
        map->seed = 0xAA6F3B22; /* 0xAA6F3B22 is the MurmurHash3_32(seed = 0) result of somebody's name :) */
    } else {
        map->seed = seed;
    }
    map->hash = murmur3_32;
    map->cmp = cmp;
    map->key_dup = key_dup;
    map->key_free = key_free;
    map->value_dup = value_dup;
    map->value_free = value_free;
    map->size_of = size_of_key;
    map->buckets = clog_hashmap_create_empty_buckets(map->buckets_size);
    if (map->buckets == NULL) {
        clog_free(map);
        return NULL;
    }
    return map;
}

static void clog_hashmap_free_entry(const clog_hashmap_t* map, clog_hashmap_entry_t* entry)
{
    clog_hashmap_free_key(map, entry->key);
    clog_hashmap_free_value(map, entry->value);
    clog_free(entry);
}

static void clog_hashmap_free_buckets(const clog_hashmap_t* map, clog_hashmap_entry_t* buckets, const size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        clog_hashmap_entry_t* entry = buckets[i].next;
        while (entry != NULL) {
            clog_hashmap_entry_t* next = entry->next;
            clog_hashmap_free_entry(map, entry);
            entry = next;
        }
    }
    clog_free(buckets);
}

static clog_res_e clog_hashmap_put_internal(clog_hashmap_t* map, void* key, void* value, bool* is_replace)
{
    clog_hashmap_entry_t* bucket = &map->buckets[clog_hashmap_get_index(map, key)];
    clog_hashmap_entry_t* entry = bucket->next;
    while (entry != NULL) {
        if (clog_hashmap_cmp(map, key, entry->key)) {
            /* replace and free old value */
            clog_hashmap_free_value(map, entry->value);
            entry->value = value; /* no need to dup */
            *is_replace = true;
            return CLOG_SUCCESS;
        }
        entry = entry->next;
    }

    /* insert new entry */
    clog_hashmap_entry_t* new_entry = clog_malloc(sizeof(clog_hashmap_entry_t));
    CLOG_RET_IF_NULL(new_entry, CLOG_NO_MEMORY);
    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = bucket->next;
    if (new_entry->key == NULL || new_entry->value == NULL) {
        clog_hashmap_free_entry(map, new_entry);
        return CLOG_NO_MEMORY;
    }
    bucket->next = new_entry;
    map->data_size++;
    return CLOG_SUCCESS;
}

static clog_res_e clog_hashmap_resize(clog_hashmap_t* map, const size_t new_size)
{
    clog_hashmap_entry_t* new_entries = clog_hashmap_create_empty_buckets(new_size);
    CLOG_RET_IF_NULL(new_entries, CLOG_NO_MEMORY);

    /* store old bucket, rollback if resize failed */
    clog_hashmap_entry_t* old_entries = map->buckets;
    const size_t old_size = map->buckets_size;
    const size_t old_data_size = map->data_size;
    /* setup new buckets */
    map->buckets = new_entries;
    map->buckets_size = new_size;
    map->data_size = 0;
    /* put old buckets into new buckets */
    for (size_t i = 0; i < old_size; ++i) {
        const clog_hashmap_entry_t* entry = old_entries[i].next;
        if (entry == NULL) {
            continue;
        }

        while (entry != NULL) {
            /**
             * it is impossible for the same key to appear in the old buckets
             * there is no need to handle the replacement scenario here
             * if clog_hashmap_put_internal failed, rollback to old buckets
             */
            bool is_replace = false;
            if (clog_hashmap_put_internal(map, entry->key, entry->value, &is_replace) != CLOG_SUCCESS) {
                map->buckets = old_entries;
                map->buckets_size = old_size;
                map->data_size = old_data_size;
                clog_free(new_entries);
                return CLOG_FAIL;
            }
            entry = entry->next;
        }
    }

    /* the entries of old buckets have been moved to new buckets, so just free old_entries */
    clog_free(old_entries);
    return CLOG_SUCCESS;
}

clog_res_e clog_hashmap_put(clog_hashmap_t* map, const void* key, const void* value)
{
    CLOG_RET_IF_NULL(map, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(key, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(value, CLOG_INVALID_PARAM);
    void* new_key = clog_hashmap_dup_key(map, key);
    CLOG_RET_IF_NULL(new_key, CLOG_NO_MEMORY);
    void* new_value = clog_hashmap_dup_value(map, value);
    if (new_value == NULL) {
        clog_hashmap_free_key(map, new_key);
        return CLOG_NO_MEMORY;
    }

    if (map->data_size >= map->buckets_size) { /* attempt to resize the hashmap */
        CLOG_IGNORE_RES(clog_hashmap_resize(map, map->buckets_size * 2));
    }

    bool is_replace = false;
    const clog_res_e ret = clog_hashmap_put_internal(map, new_key, new_value, &is_replace);
    if (ret != CLOG_SUCCESS) {
        clog_hashmap_free_key(map, new_key);
        clog_hashmap_free_value(map, new_value);
    } else {
        if (is_replace) {
            clog_hashmap_free_key(map, new_key);
        }
    }

    return ret;
}

const void* clog_hashmap_get(const clog_hashmap_t* map, const void* key)
{
    CLOG_RET_IF_NULL(map, NULL);
    CLOG_RET_IF_NULL(key, NULL);

    clog_hashmap_entry_t* bucket = &map->buckets[clog_hashmap_get_index(map, key)];
    clog_hashmap_entry_t* last = bucket;
    clog_hashmap_entry_t* entry = last->next;
    while ((entry != NULL) && (!clog_hashmap_cmp(map, entry->key, key))) {
        last = entry;
        entry = entry->next;
    }
    CLOG_RET_IF_NULL(entry, NULL);
    if (bucket->next != entry) { /* move the most recently used entry to the front */
        last->next = entry->next;
        entry->next = bucket->next;
        bucket->next = entry;
    }
    return entry->value;
}

void* clog_hashmap_take(clog_hashmap_t* map, const void* key)
{
    CLOG_RET_IF_NULL(map, NULL);
    CLOG_RET_IF_NULL(key, NULL);

    clog_hashmap_entry_t* bucket = &map->buckets[clog_hashmap_get_index(map, key)];
    clog_hashmap_entry_t* last = bucket;
    clog_hashmap_entry_t* entry = last->next;
    while ((entry != NULL) && (!clog_hashmap_cmp(map, entry->key, key))) {
        last = entry;
        entry = entry->next;
    }
    CLOG_RET_IF_NULL(entry, NULL);
    void* value = entry->value;
    last->next = entry->next; /* remove this entry */
    clog_hashmap_free_key(map, entry->key);
    clog_free(entry);
    map->data_size--;
    return value;
}

void* clog_hashmap_get_dup(const clog_hashmap_t* map, const void* key)
{
    const void* value = clog_hashmap_get(map, key);
    CLOG_RET_IF_NULL(value, NULL);
    return clog_hashmap_dup_value(map, value);
}

bool clog_hashmap_remove(clog_hashmap_t* map, const void* key)
{
    if (map == NULL || map->data_size == 0) {
        return false;
    }
    clog_hashmap_entry_t* last = &map->buckets[clog_hashmap_get_index(map, key)];
    clog_hashmap_entry_t* entry = last->next;
    CLOG_RET_IF_NULL(entry, false);

    while (entry != NULL && !clog_hashmap_cmp(map, entry->key, key)) {
        last = entry;
        entry = entry->next;
    }

    CLOG_RET_IF_NULL(entry, false);

    map->data_size--;

    last->next = entry->next;
    clog_hashmap_free_entry(map, entry);
    const size_t new_size = map->buckets_size / 2;
    if (map->data_size < new_size) {
        /* if the occupation is less than half, release excess memory */
        CLOG_IGNORE_RES(clog_hashmap_resize(map, new_size));
    }
    return true;
}

bool clog_hashmap_is_exists(const clog_hashmap_t* map, const void* key)
{
    if (map == NULL) {
        return false;
    }

    clog_hashmap_entry_t* entry = map->buckets[clog_hashmap_get_index(map, key)].next;
    CLOG_RET_IF_NULL(entry, false);

    while ((entry != NULL) && (!clog_hashmap_cmp(map, entry->key, key))) {
        entry = entry->next;
    }

    return entry != NULL;
}

size_t clog_hashmap_size(const clog_hashmap_t* map)
{
    CLOG_RET_IF_NULL(map, 0);
    return map->data_size;
}

void clog_hashmap_clear(clog_hashmap_t* map)
{
    CLOG_RET_VOID_IF_NULL(map);

    for (int i = 0; i < map->buckets_size; i++) {
        clog_hashmap_entry_t* entry = map->buckets[i].next;
        clog_hashmap_entry_t* next = NULL;
        while (entry != NULL) {
            next = entry->next;
            clog_hashmap_free_entry(map, entry);
            entry = next;
        }
        map->buckets[i].next = NULL;
    }

    map->data_size = 0;
}

void clog_hashmap_destroy(clog_hashmap_t** map)
{
    if (map == NULL || *map == NULL) {
        return;
    }

    clog_hashmap_free_buckets(*map, (*map)->buckets, (*map)->buckets_size);
    clog_free(*map);
    *map = NULL;
}
