/**
 * @author Polaris
 * @date  2025/9/12
 */
#include "clog_config.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clog_hooks.h"
#include "clog_secure_func.h"

static clog_config_group_t* clog_config_create_empty_group(void)
{
    clog_config_group_t* group = clog_malloc(sizeof(clog_config_group_t));
    CLOG_RET_IF_NULL(group, NULL);
    group->name = NULL;
    group->child = NULL;
    group->sibling = NULL;
    group->content = NULL;
    return group;
}

static clog_config_item_t* clog_config_create_empty_item(void)
{
    clog_config_item_t* item = clog_malloc(sizeof(clog_config_item_t));
    CLOG_RET_IF_NULL(item, NULL);
    item->key = NULL;
    item->type = CLOG_CONFIG_ITEM_TYPE_INVALID;
    item->value.str = NULL;
    item->next = NULL;
    return item;
}


static void clog_config_free_item(clog_config_item_t* item)
{
    CLOG_RET_VOID_IF_NULL(item);
    clog_free((void*)item->key);
    if (item->type == CLOG_CONFIG_ITEM_TYPE_STRING) {
        clog_free(item->value.str);
    }
    clog_free(item);
}

void clog_config_destroy_group(clog_config_group_t* group)
{
    CLOG_RET_VOID_IF_NULL(group);
    clog_config_group_t* child = group->child;
    clog_config_group_t* tmp = NULL;
    while (child != NULL) {
        tmp = child->sibling;
        clog_config_destroy_group(child);
        child = tmp;
    }
    group->child = NULL;
    group->sibling = NULL;
    clog_free((void*)group->name);
    clog_config_item_t* item = group->content;
    clog_config_item_t* next = NULL;
    while (item != NULL) {
        next = item->next;
        clog_config_free_item(item);
        item = next;
    }
    clog_free(group);
}

static bool clog_config_is_valid_name_char(const char ch)
{
    if (ch == '_' || (ch >= '0' && ch <= '9')) {
        return true;
    }
    if (ch >= 'a' && ch <= 'z') {
        return true;
    }
    if (ch >= 'A' && ch <= 'Z') {
        return true;
    }
    return false;
}

/**
 * find or create group from root
 * @param r root group
 * @param start start char of valid group name, e.g, [ ad.bc ], start means 'a'
 * @param end end char of valid group name, e.g, [ ad.bc ], end means 'c'
 * @return parsed group
 */
static clog_config_group_t* clog_config_find_or_create_group(clog_config_group_t* r, const char* start, const char* end)
{
    const char* name_start = start;
    const char* name_end = start;
    while (name_end < end) {
        if (*name_end == '.') {
            name_end = name_end - 1;
            break;
        }
        name_end++;
    }

    clog_config_group_t* current = r->child;
    if (current == NULL) { /* no sub nodes, create */
        clog_config_group_t* new_group = clog_config_create_empty_group();
        CLOG_RET_IF_NULL(new_group, NULL);
        new_group->name = clog_strndup(name_start, name_end - name_start + 1);
        if (new_group->name == NULL) {
            clog_config_destroy_group(new_group);
            return NULL;
        }
        r->child = new_group;
        if (name_end == end) {
            return new_group;
        }
        return clog_config_find_or_create_group(r->child, name_end + 2, end);
    }

    clog_config_group_t* last = current;
    while (current != NULL) {
        if (clog_strncmp(current->name, name_start, name_end - name_start + 1) == 0) {
            CLOG_RET_IF(name_end == end, current); /* same name group found, and there is no more subgroup */
            return clog_config_find_or_create_group(current, name_end + 2, end); /* parse subgroup */
        }
        last = current;
        current = current->sibling; /* find next sibling */
    }
    clog_config_group_t* new_group = clog_config_create_empty_group();
    CLOG_RET_IF_NULL(new_group, NULL);
    new_group->name = clog_strndup(name_start, name_end - name_start + 1);
    if (new_group->name == NULL) {
        clog_config_destroy_group(new_group);
        return NULL;
    }
    last->sibling = new_group;
    CLOG_RET_IF(name_end == end, new_group);
    return clog_config_find_or_create_group(new_group, name_end + 2, end); /* skip . */
}

/* [ abc.def ] -> abc.def */
static bool clog_config_get_valid_group_name(const char* start, const char* end, const char** format_start,
                                             const char** format_end)
{
    const char* tmp = start + 1;
    while (*tmp == ' ') {
        tmp++; /* remove space between '[' and group name */
    }
    if (tmp == end) {
        return false; /* no ']' found, e.g. "[   " */
    }
    if (*tmp == ']') {
        return false; /* no valid group name, e.g. " [  ]" */
    }
    if (*tmp == '.') {
        return false; /* no valid group name before '.', e.g. "[.xx]" */
    }
    *format_start = tmp;
    *format_end = tmp;
    bool is_split_exist = false;
    bool is_parsed_end = false;
    while (tmp < end) {
        if (is_parsed_end) {
            if (*tmp == '#') {
                break;
            }
            if (*tmp != ' ') {
                return false; /* other characters (except space) after ']'  */
            }
            tmp++;
            continue;
        }
        if (*tmp == '.') {
            if (is_split_exist) {
                return false; /* two consecutive '.' */
            }
            is_split_exist = true;
        } else if (*tmp == ']') {
            is_split_exist = false;
            is_parsed_end = true;
            *format_end = tmp - 1;
        } else if (clog_config_is_valid_name_char(*tmp)) {
            is_split_exist = false;
        } else {
            return false; /* invalid char */
        }
        tmp++;
    }
    if (*format_start == *format_end) {
        return false; /* no ']' found, e.g. "[ xxx" */
    }
    return true;
}

static bool clog_config_check_inline_comment(const char* start, const char* end)
{
    CLOG_RET_IF(start == end, true);
    const char* tmp = start;
    while (*tmp == ' ') {
        tmp++;
    }
    CLOG_RET_IF(tmp == end, true);
    return *tmp == '#';
}

static bool clog_config_parse_value_char(const char* start, const char* end, clog_config_item_t* item)
{
    const char* tmp = start + 1;
    char value = '\0';
    CLOG_RET_IF(tmp == end, false); /* only single quote existed */
    if (*tmp == '\\') {
        tmp++;
        CLOG_RET_IF(tmp == end, false); /* only single quote and escape char existed */
        if (*tmp == '\'') {
            value = '\'';
        } else if (*tmp == 'n') {
            value = '\n';
        } else if (*tmp == 't') {
            value = '\t';
        } else if (*tmp == 'r') {
            value = '\r';
        } else if (*tmp == '\\') {
            value = '\\';
        } else if (*tmp == '0') {
            value = '\0';
        } else {
            return false; /* invalid escape char */
        }
    } else {
        value = *tmp;
    }
    tmp++;
    CLOG_RET_IF(tmp == end, false); /* right single quote not found */
    CLOG_RET_IF(*tmp != '\'', false); /* only single character is allowed between single quotation marks */
    tmp++;
    CLOG_RET_IF(!clog_config_check_inline_comment(tmp, end), false);
    item->value.ch = value;
    item->type = CLOG_CONFIG_ITEM_TYPE_CHAR;
    return true;
}

static bool clog_config_parse_value_uint_radix(const uint8_t radix, const char* start, const char* end,
                                               clog_config_item_t* item)
{
    const char* tmp = start;
    const char* number_end = end;
    while (tmp < end) {
        bool valid = false;
        if (*tmp == ' ' || *tmp == '#') {
            break;
        }
        switch (radix) {
            case 2:
                valid = *tmp == '0' || *tmp == '1';
                break;
            case 8:
                valid = *tmp >= '0' && *tmp <= '7';
                break;
            case 10:
                valid = *tmp >= '0' && *tmp <= '9';
                break;
            case 16:
                valid = (*tmp >= '0' && *tmp <= '9') || (*tmp >= 'a' && *tmp <= 'f') || (*tmp >= 'A' && *tmp <= 'F');
                break;
            default:
                return false; /* never go here */
        }
        if (!valid) {
            return false;
        }
        number_end = tmp;
        tmp++;
    }
    CLOG_RET_IF(!clog_config_check_inline_comment(tmp, end), false);
    tmp = number_end;
    uint32_t base = 1;
    uint32_t res = 0;
    while (true) {
        const uint32_t remain = UINT32_MAX / base;
        const uint8_t digit = *tmp - '0';
        if (remain < digit) {
            return false; /* overflow */
        }
        const uint32_t current = digit * base;
        if (UINT32_MAX - current < res) {
            return false; /* overflow */
        }
        res += current;
        tmp--;
        if (tmp < start) {
            item->value.uint = res;
            return true;
        }
        if (remain < radix) {
            return false; /* overflow */
        }
        base *= radix;
    }
}

/* +0.123. -0.123, 0.123 */
static bool clog_config_parse_value_double(const char* start, const char* end, clog_config_item_t* item)
{
    bool is_dot = false;
    const char* tmp = start;
    if (*tmp == '+' || *tmp == '-') {
        tmp++;
    }
    const char* number_end = start;
    while (tmp < end) {
        if (*tmp == '.') {
            CLOG_RET_IF(is_dot, false); /* only one '.' allowed */
            is_dot = true;
        } else if (*tmp == ' ' || *tmp == '#') {
            break;
        } else if (*tmp < '0' || *tmp > '9') {
            return false; /* invalid char */
        } else {
            number_end = tmp;
        }
        tmp++;
    }
    CLOG_RET_IF(!clog_config_check_inline_comment(tmp, end), false);
    char* str = clog_strndup(start, number_end - start + 1);
    CLOG_RET_IF_NULL(str, false);
    errno = 0;
    const double res = strtod(str, NULL);
    CLOG_RET_IF(errno != 0, false);
    clog_free(str);
    item->type = CLOG_CONFIG_ITEM_TYPE_FLOAT;
    item->value.f = res;
    return true;
}

static uint8_t clog_config_get_radix(const char ch)
{
    if (ch == 'x') {
        return 16; /* "0x..." */
    }
    if (ch == 'o') {
        return 8; /* "0o..." */
    }
    if (ch == 'b') {
        return 2; /* "0b..." */
    }
    return 10;
}

static bool clog_config_parse_value_number(const char* start, const char* end, clog_config_item_t* item)
{
    bool is_negative = false;
    bool is_flag_exist = false;
    item->type = CLOG_CONFIG_ITEM_TYPE_UINT;
    const char* tmp = start;
    const char* number_start = start;
    if (*tmp == '-' || *tmp == '+') {
        is_negative = *tmp == '-';
        is_flag_exist = true;
        tmp++;
        item->type = CLOG_CONFIG_ITEM_TYPE_INT;
        number_start = tmp;
    }
    if (*tmp == '0') {
        if (tmp + 1 == end) { /* "+0", "0", "-0" */
            if (item->type == CLOG_CONFIG_ITEM_TYPE_INT) {
                item->value.sint = 0;
            } else {
                item->value.uint = 0;
            }
            return true;
        }
        tmp++;
        if (*tmp == '.') {
            tmp--;
            if (is_flag_exist) {
                tmp--;
            }
            return clog_config_parse_value_double(tmp, end, item);
        }
        const uint8_t radix = clog_config_get_radix(*tmp);
        if (radix != 10) {
            CLOG_RET_IF(is_flag_exist, false); /* '+','-' can't be before "0b", "0o", "0x" */
            tmp++;
            CLOG_RET_IF(tmp == end || *tmp == ' ', false); /* no valid number, e.g. "0x", "0x " */
            return clog_config_parse_value_uint_radix(radix, tmp, end, item);
        }
    }
    CLOG_RET_IF(*tmp == '.', false); /* no number before '.', e.g. ".123", "-.0" */
    while (*tmp != ' ' && *tmp != '#' && tmp < end) {
        if (*tmp == '.') {
            return clog_config_parse_value_double(start, end, item); /* parse with '+'/'-' */
        }
        if (*tmp < '0' || *tmp > '9') {
            return false; /* invalid char */
        }
        tmp++;
    }
    CLOG_RET_IF(!clog_config_parse_value_uint_radix(10, number_start, end, item), false); /* parse without '+'/'-' */
    if (is_negative) { /* negative */
        CLOG_RET_IF(item->value.uint > INT32_MAX, false); /* overflow */
        item->value.sint = (int32_t)(item->value.uint * -1);
    }
    return true;
}

static bool clog_config_parse_value_string(const char* start, const char* end, clog_config_item_t* item)
{
    const char* tmp = start;

    char* result = clog_malloc(end - start + 1);
    CLOG_RET_IF_NULL(result, false);
    char* out = result;
    while (*tmp == ' ') {
        tmp++;
    }
    if (tmp >= end || *tmp != '"') { /* no start with '"' */
        clog_free(result);
        return false;
    }
    tmp++;

    bool is_escaping = false;
    bool parsed_end = false;
    while (tmp < end) {
        if (is_escaping) {
            if (*tmp == '\\' || *tmp == '"') {
                *out = *tmp;
                out++;
                is_escaping = false;
            } else {
                break;
            }
        } else {
            if (*tmp == '\\') {
                is_escaping = true;
            } else if (*tmp == '"') {
                parsed_end = true;
                tmp++; /* meet enclose '"' */
                break;
            } else {
                *out = *tmp;
                out++;
            }
        }
        tmp++;
    }
    if (!parsed_end || !clog_config_check_inline_comment(tmp, end)) {
        clog_free(result);
        return false;
    }
    *out = '\0';
    item->value.str = result;
    item->type = CLOG_CONFIG_ITEM_TYPE_STRING;
    return true;
}

static bool clog_config_parse_value_bool(const char* start, const char* end, clog_config_item_t* item)
{
    const char* tmp = start;
    while (*tmp == ' ') {
        tmp++;
    }
    CLOG_RET_IF(start >= end, false);
    if (strncmp(tmp, "true", 4) == 0) {
        tmp += 4;
        item->value.flag = true;
    } else if (strncmp(tmp, "false", 5) == 0) {
        tmp += 5;
        item->value.flag = false;
    } else {
        return false;
    }
    CLOG_RET_IF(!clog_config_check_inline_comment(tmp, end), false);
    item->type = CLOG_CONFIG_ITEM_TYPE_BOOL;
    return true;
}

static bool clog_config_parse_value(const char* start, const char* end, clog_config_item_t* item)
{
    const char* tmp = start;
    while (*tmp == ' ') {
        tmp++; /* remove leading spaces */
    }
    if (tmp == end) {
        return false; /* empty */
    }
    if (*tmp == '+' || *tmp == '-' || (*tmp >= '0' && *tmp <= '9')) {
        return clog_config_parse_value_number(tmp, end, item);
    }
    if (*tmp == '\'') {
        return clog_config_parse_value_char(tmp, end, item);
    }
    if (*tmp == '"') {
        return clog_config_parse_value_string(tmp, end, item);
    }
    if (*tmp == 't' || *tmp == 'f') {
        return clog_config_parse_value_bool(tmp, end, item);
    }
    return false;
}

static bool clog_config_add_item(clog_config_group_t* group, clog_config_item_t* item)
{
    if (group->content == NULL) {
        group->content = item;
        return true;
    }
    const clog_config_item_t* tmp = group->content;
    while (tmp != NULL) {
        if (strcmp(tmp->key, item->key) == 0) {
            return false;
        }
        tmp = tmp->next;
    }
    item->next = group->content;
    group->content = item;
    return true;
}

static clog_config_item_t* clog_config_parse_content(const char* start, const char* end)
{
    enum { PARSE_KEY, PARSE_EQUAL, PARSE_VALUE };
    clog_config_item_t* item = clog_config_create_empty_item();
    CLOG_RET_IF_NULL(item, NULL);
    const char* name_start = start;
    const char* name_end = start;
    const char* tmp = start + 1;
    uint32_t phase = PARSE_KEY;
    while (tmp < end) {
        switch (phase) {
            case PARSE_KEY:
                if (*tmp == ' ' || *tmp == '=') {
                    item->key = clog_strndup(name_start, name_end - name_start + 1);
                    if (item->key == NULL) {
                        clog_free(item);
                        return NULL;
                    }
                    phase = *tmp == ' ' ? PARSE_EQUAL : PARSE_VALUE;
                } else if (!clog_config_is_valid_name_char(*tmp)) {
                    return false;
                } else {
                    /* valid name, continue to parse */
                    name_end = tmp;
                }
                break;
            case PARSE_EQUAL:
                if (*tmp == '=') {
                    phase = PARSE_VALUE;
                } else if (*tmp != ' ') {
                    return false; /* invalid char */
                } else {
                    /* continue to parse */
                }
                break;
            case PARSE_VALUE:
                CLOG_RET_IF(clog_config_parse_value(tmp, end, item), item);
                /* fall-through */
            default:
                clog_config_free_item(item);
                return NULL;
        }
        tmp++;
    }
    clog_config_free_item(item);
    return NULL;
}

static clog_config_group_t* clog_config_format_line(const char* start, const char* end, clog_config_group_t* root,
                                                    clog_config_group_t* current)
{
    const char* tmp = start;
    while (*tmp == ' ') {
        tmp++; /* skip leading spaces at the beginning of the line */
    }
    if (tmp == end) {
        return current; /* empty line */
    }
    if (*tmp == '[') {
        const char* group_start = NULL;
        const char* group_end = NULL;
        if (!clog_config_get_valid_group_name(tmp, end, &group_start, &group_end)) {
            return NULL;
        }
        return clog_config_find_or_create_group(root, group_start, group_end);
    }
    if (*tmp == '#') {
        return current;
    }
    if (clog_config_is_valid_name_char(*tmp)) {
        clog_config_item_t* item = clog_config_parse_content(tmp, end);
        CLOG_RET_IF_NULL(item, NULL);
        CLOG_RET_IF(clog_config_add_item(current, item), current);
        clog_config_free_item(item);
        return NULL;
    }
    return NULL;
}

clog_config_group_t* clog_config_parse(const char* data, char* err, const size_t size)
{
    CLOG_RET_IF_NULL(data, NULL);
    clog_config_group_t* root = clog_config_create_empty_group();
    clog_config_group_t* current = root; /* current parsing group */
    CLOG_RET_IF_NULL(root, NULL);
    const char* tmp = data;
    while (*tmp != '\0') {
        const char* start = tmp; /* start of a line */
        while (*tmp != '\n' && *tmp != '\r' && *tmp != '\0') {
            tmp++;
        }
        const char* end = tmp; /* end of a line, \n \r or \0 */
        if (start != end) {
            /* start to end indicate a line, end char is not included */
            current = clog_config_format_line(start, end, root, current);
            if (current == NULL) {
                CLOG_RET_IF_NULL(err, NULL);
                char* line = clog_strndup(start, end - start + 1);
                (void)snprintf(err, size, "[%s] format error", line == NULL ? "DUPLICATE FAILED" : line);
                clog_free(line);
                clog_config_destroy_group(root);
                return NULL;
            }
        }

        while (*tmp == '\n' || *tmp == '\r') {
            tmp++; /* move to start of next line */
        }
    }
    return root;
}

static size_t clog_config_dump_set_indent(char* buf, const size_t size, const size_t level)
{
    const size_t num = level;
    CLOG_RET_IF(num >= size, 0);
    for (size_t i = 0; i < num; i++) {
        buf[i] = '\t';
    }
    return num;
}

static size_t clog_config_dump_item(const clog_config_item_t* item, char* buf, const size_t size, const size_t level)
{
    const size_t offset = clog_config_dump_set_indent(buf, size, level);
    CLOG_RET_IF(offset == 0, 0);

    int len = 0;
    switch (item->type) {
        case CLOG_CONFIG_ITEM_TYPE_INT:
            len = snprintf(buf + offset, size - offset, "%s=%d\n", item->key, item->value.sint);
            break;
        case CLOG_CONFIG_ITEM_TYPE_UINT:
            len = snprintf(buf + offset, size - offset, "%s=%u\n", item->key, item->value.uint);
            break;
        case CLOG_CONFIG_ITEM_TYPE_FLOAT:
            len = snprintf(buf + offset, size - offset, "%s=%lf\n", item->key, item->value.f);
            break;
        case CLOG_CONFIG_ITEM_TYPE_CHAR:
            len = snprintf(buf + offset, size - offset, "%s=%c\n", item->key, item->value.ch);
            break;
        case CLOG_CONFIG_ITEM_TYPE_STRING:
            len = snprintf(buf + offset, size - offset, "%s=%s\n", item->key, item->value.str);
            break;
        case CLOG_CONFIG_ITEM_TYPE_BOOL:
            len = snprintf(buf + offset, size - offset, "%s=%s\n", item->key, item->value.flag ? "true" : "false");
            break;
        default:
            return 0;
    }
    CLOG_RET_IF(len <= 0 || (size_t)len >= size - offset, 0);
    return offset + len;
}

const clog_config_item_t* clog_config_find_item(const clog_config_group_t* root, const char* groups[],
                                                const size_t count, const char* key)
{
    CLOG_RET_IF_NULL(root, NULL);
    CLOG_RET_IF_NULL(key, NULL);
#if CASCA_LOG_DEBUG == 0
    CLOG_RET_IF(count > 1024, NULL); /* attention stack overflow */
#endif
    if (count == 0) { /* find key in root itself */
        const clog_config_item_t* item = root->content;
        while (item != NULL) {
            if (strcmp(item->key, key) == 0) {
                return item;
            }
            item = item->next;
        }
        return NULL;
    }
    CLOG_RET_IF(groups[0] == NULL, NULL);
    const clog_config_group_t* group = root->child;
    while (group != NULL) {
        if (strcmp(group->name, groups[0]) == 0) {
            return clog_config_find_item(group, groups + 1, count - 1, key);
        }
        group = group->sibling;
    }
    return NULL;
}


static size_t clog_config_dump_group_internal(const clog_config_group_t* group, char* buf, const size_t size,
                                              const size_t level)
{
    size_t offset = clog_config_dump_set_indent(buf, size, level);
    CLOG_RET_IF(offset == 0 && level > 0, 0);
    if (group->name != NULL) {
        const size_t len = snprintf(buf + offset, size - offset, "[%s]\n", group->name);
        CLOG_RET_IF(len <= 0 || len >= size - offset, 0);
        offset += len;
    }

    const clog_config_item_t* item = group->content;
    while (item != NULL) {
        const size_t len = clog_config_dump_item(item, buf + offset, size - offset, level + 1);
        CLOG_RET_IF(len == 0, 0);
        offset += len;
        item = item->next;
    }
    const clog_config_group_t* child = group->child;
    while (child != NULL) {
        const size_t len = clog_config_dump_group_internal(child, buf + offset, size - offset, level + 1);
        CLOG_RET_IF(len == 0, 0);
        offset += len;
        child = child->sibling;
    }
    CLOG_RET_IF(offset >= size, 0);
    buf[offset] = '\0';
    return offset;
}

bool clog_config_dump_group(const clog_config_group_t* group, char* buf, const size_t size)
{
    CLOG_RET_IF_NULL(group, false);
    CLOG_RET_IF_NULL(buf, false);
    return clog_config_dump_group_internal(group, buf, size, 0) != 0;
}
