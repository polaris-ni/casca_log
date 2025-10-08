/**
 * @author Polaris
 * @date  2025/10/2
 */
#ifndef CASCA_LOG_CLOG_HASHMAP_H
#define CASCA_LOG_CLOG_HASHMAP_H

#include <stdbool.h>
#include <stdint.h>
#include "casca_log_defines.h"
#include "clog_secure_func.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct clog_hashmap_entry clog_hashmap_entry_t;

struct clog_hashmap_entry {
    void* key;
    void* value;
    clog_hashmap_entry_t* next;
};

struct clog_hashmap;
typedef struct clog_hashmap clog_hashmap_t;

typedef bool (*clog_hashmap_cmp_f)(const void* key1, const void* key2);

typedef void* (*clog_hashmap_dup_f)(const void* ptr);

typedef void (*clog_hashmap_free_f)(void* ptr);

typedef size_t (*clog_hashmap_size_f)(const void* ptr);

/**
 * create hashmap
 * @param key_size if size of key is fixed, #key_size indicates the exact size. otherwise it should be set to 0
 * @param value_size if size of value is fixed, #value_size indicates the exact size. otherwise it should be set to 0
 * @param key_dup if #key_size is not 0, it should be NULL. otherwise it should be a function to duplicate key
 * @param key_free if #key_size is not 0, it should be NULL. otherwise it should be a function to free key
 * @param value_dup if #value_size is not 0, it should be NULL. otherwise it should be a function to duplicate value
 * @param value_free if #value_size is not 0, it should be NULL. otherwise it should be a function to free value
 * @param cmp if #key_size is not 0, it should be NULL. otherwise it should be a function to compare key
 * @param size_of_key if #key_size is not 0, it should be NULL. otherwise it should be a function to get size of key
 * @param seed seed used in hash function, if seed is 0, default seed 0xAA6F3B22 will be applied
 * @return created hashmap
 */
clog_hashmap_t* clog_hashmap_create(size_t key_size, size_t value_size, clog_hashmap_dup_f key_dup,
                                    clog_hashmap_free_f key_free, clog_hashmap_dup_f value_dup,
                                    clog_hashmap_free_f value_free, clog_hashmap_cmp_f cmp,
                                    clog_hashmap_size_f size_of_key, uint32_t seed);

/**
 * put key-value into hashmap, if key exists, value will be replaced
 * @param map #clog_hashmap_t
 * @param key key
 * @param value value
 * @return #clog_res_e
 */
clog_res_e clog_hashmap_put(clog_hashmap_t* map, const void* key, const void* value);

/**
 * get value by key, not thread safe
 * @param map #clog_hashmap_t
 * @param key key
 * @return the original value ptr of key, the caller should not free it, and there may be concurrent conflicts
 */
const void* clog_hashmap_get(const clog_hashmap_t* map, const void* key);

/**
 * get value by key, and the key-value will be removed
 * @param map #clog_hashmap_t
 * @param key key
 * @return the original value ptr of key, the caller should free it by appropriate #clog_hashmap_free_f
 */
void* clog_hashmap_take(clog_hashmap_t* map, const void* key);

/**
 * get duplicated value by key
 * @param map #clog_hashmap_t
 * @param key key
 * @return the duplicated value, the caller should free it by appropriate #clog_hashmap_free_f
 */
void* clog_hashmap_get_dup(const clog_hashmap_t* map, const void* key);

/**
 * remove key-value pair
 * @param map #clog_hashmap_t
 * @param key key
 * @return true if remove success, false if key does not exist
 */
bool clog_hashmap_remove(clog_hashmap_t* map, const void* key);

/**
 * check if key exists
 * @param map #clog_hashmap_t
 * @param key key
 * @return true if key exists, false otherwise
 */
bool clog_hashmap_is_exists(const clog_hashmap_t* map, const void* key);

/**
 * get size of hashmap
 * @param map #clog_hashmap_t
 * @return size of hashmap
 */
size_t clog_hashmap_size(const clog_hashmap_t* map);

/**
 * clear all key-value pairs
 * @param map #clog_hashmap_t
 */
void clog_hashmap_clear(clog_hashmap_t* map);

/**
 * destroy hashmap
 * @param map #clog_hashmap_t, *map will be set to NULL
 */
void clog_hashmap_destroy(clog_hashmap_t** map);

/**
 * duplicate string
 * @param str string to duplicate
 * @return duplicated string
 */
static void* clog_hashmap_string_dup(const void* str)
{
    return clog_strdup((const char *)str);
}

/**
 * free string
 * @param str string to be free
 */
static void clog_hashmap_string_free(void* str)
{
    clog_free(str);
}

/**
 * compare string
 * @param str1 string1
 * @param str2 string2
 * @return true if str1 == str2, false otherwise
 */
static bool clog_hashmap_string_cmp(const void* str1, const void* str2)
{
    return strcmp((const char *)str1, (const char *)str2) == 0;
}

/**
 * get size of string
 * @param str string
 * @return size of string
 */
static size_t clog_hashmap_string_size(const void* str)
{
    return strlen((const char *)str);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* CASCA_LOG_CLOG_HASHMAP_H */
