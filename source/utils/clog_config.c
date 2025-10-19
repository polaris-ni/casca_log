/**
 * @author Polaris
 * @date  2025/9/12
 */
#include "clog_config.h"
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clog_error.h"
#include "clog_secure_func.h"

static const char* clog_config_remove_leading_spaces(const char* start, const char* end)
{
    const char* tmp = start;
    while (tmp < end) {
        if ((*tmp != ' ') && (*tmp != '\t')) {
            return tmp;
        }
        tmp++;
    }
    return tmp;
}

static clog_config_group_t* clog_config_create_empty_group(void)
{
    clog_config_group_t* group = clog_malloc(sizeof(clog_config_group_t));
    CLOG_RET_IF_NULL_X(group, NULL, "malloc new empty group failed");
    group->name = NULL;
    group->child = NULL;
    group->sibling = NULL;
    group->content = NULL;
    return group;
}

static clog_config_item_t* clog_config_create_empty_item(void)
{
    clog_config_item_t* item = clog_malloc(sizeof(clog_config_item_t));
    CLOG_RET_IF_NULL_X(item, NULL, "malloc new empty item failed");
    item->key = NULL;
    item->type = CLOG_CONFIG_TYPE_INVALID;
    item->value.str = NULL;
    item->next = NULL;
    return item;
}

static void clog_config_free_item(clog_config_item_t* item)
{
    CLOG_RET_VOID_IF_NULL(item);
    clog_free((void*)item->key);
    if (item->type == CLOG_CONFIG_TYPE_STRING) {
        clog_free(item->value.str);
    }
    if (item->type == CLOG_CONFIG_TYPE_ARRAY) {
        clog_config_item_t* array = item->value.array;
        while (array != NULL) {
            clog_config_item_t* next = array->next;
            clog_config_free_item(array);
            array = next;
        }
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
    /* " abc .def": name_start -> 'a', name_end -> ' ' */
    const char* name_start = clog_config_remove_leading_spaces(start, end);
    const char* name_end = name_start;
    while (name_end < end) {
        if (*name_end == '.' || *name_end == ' ' || *name_end == '\t') {
            break;
        }
        name_end++;
    }

    if (r->child == NULL) { /* no sub nodes, create */
        clog_config_group_t* new_group = clog_config_create_empty_group();
        CLOG_RET_IF_NULL(new_group, NULL);
        new_group->name = clog_strndup(name_start, name_end - name_start);
        CLOG_CLEAN_RET_IF_NULL_X(new_group->name, clog_config_destroy_group(new_group), NULL, "malloc name failed");
        r->child = new_group;
        CLOG_RET_IF(name_end == end, new_group);
        name_end = clog_config_remove_leading_spaces(name_end, end); /* name_end move to next "." */
        CLOG_RET_IF(name_end >= end, new_group); /* no other subgroup */
        return clog_config_find_or_create_group(r->child, name_end + 1, end);
    }

    clog_config_group_t* current = r->child;
    clog_config_group_t* last = current;
    while (current != NULL) {
        if (clog_strncmp(current->name, name_start, name_end - name_start) == 0) {
            CLOG_RET_IF(name_end == end, current); /* same name group found, and there is no more subgroup */
            name_end = clog_config_remove_leading_spaces(name_end, end); /* name_end move to "." */
            CLOG_RET_IF(name_end >= end, current);
            return clog_config_find_or_create_group(current, name_end + 1, end); /* parse subgroup */
        }
        last = current;
        current = current->sibling; /* find next sibling */
    }
    clog_config_group_t* new_group = clog_config_create_empty_group();
    CLOG_RET_IF_NULL(new_group, NULL);
    new_group->name = clog_strndup(name_start, name_end - name_start);
    CLOG_CLEAN_RET_IF_NULL_X(new_group->name, clog_config_destroy_group(new_group), NULL, "malloc name failed");
    last->sibling = new_group;
    CLOG_RET_IF(name_end == end, new_group);
    name_end = clog_config_remove_leading_spaces(name_end, end); /* name_end move to "." */
    CLOG_RET_IF(name_end >= end, current);
    return clog_config_find_or_create_group(new_group, name_end + 1, end); /* skip . */
}

/* [ abc.def ]: *format_start -> a, *format_end -> ' ' */
static const char* clog_config_get_valid_group_name(const char* start, const char* end, const char** format_start,
                                                    const char** format_end)
{
    const char* tmp = clog_config_remove_leading_spaces(start + 1, end);
    CLOG_RET_IF(tmp == end, false); /* no ']' found, e.g. "[   " */
    CLOG_RET_IF(!clog_config_is_valid_name_char(*tmp), false); /* no valid group name, e.g. " [  ]" */
    *format_start = tmp;
    *format_end = tmp;
    bool is_split_exist = false;
    while (tmp < end) {
        if (*tmp == ' ' || *tmp == '\t') {
            tmp = clog_config_remove_leading_spaces(tmp, end);
            if (is_split_exist) {
                if (!clog_config_is_valid_name_char(*tmp)) {
                    return NULL; /* no valid name after '.', e.g. "abc. " */
                }
                is_split_exist = false;
                tmp++;
                continue;
            }
            if (*tmp == ']') {
                *format_end = tmp;
                return tmp + 1;
            }
            if (*tmp == '.') {
                return NULL; /* invalid name, e.g. "first  other" */
            }
            tmp++;
            is_split_exist = true;
            continue;
        }
        if (*tmp == '.') {
            CLOG_RET_IF(is_split_exist, NULL); /* two consecutive '.' */
            is_split_exist = true;
            tmp++;
            continue;
        }
        if (*tmp == ']') {
            CLOG_RET_IF(is_split_exist, NULL); /* no subgroup, e.g. "[ first.second. ]" */
            is_split_exist = false;
            *format_end = tmp;
            return tmp + 1;
        }
        if (clog_config_is_valid_name_char(*tmp)) {
            is_split_exist = false;
            tmp++;
            continue;
        }
        CLOG_ERR_APPEND_LINE("invalid char '%c' in group name", *tmp);
        return NULL; /* invalid char */
    }
    return NULL; /* ']' not found */
}

static bool clog_config_check_inline_comment(const char* start, const char* end)
{
    CLOG_RET_IF(start == end, true);
    const char* tmp = clog_config_remove_leading_spaces(start, end);
    CLOG_RET_IF(tmp == end, true);
    return *tmp == '#';
}

static const char* clog_config_parse_char(const char* start, const char* end, clog_config_item_t* item)
{
    const char escape_chars[] = {'\'', 'n', 't', 'r', '\\', '0'};
    const char values[] = {'\'', '\n', '\t', '\r', '\\', '\0'};
    const size_t num = CLOG_ARRAY_SIZE(escape_chars);
    CLOG_ASSERT(num == CLOG_ARRAY_SIZE(values));
    const char* tmp = start + 1;
    char value = '\0';
    CLOG_RET_IF(tmp == end, NULL); /* only single quote existed */
    if (*tmp == '\\') {
        tmp++;
        CLOG_RET_IF_X(tmp == end, NULL, "only single quote and escape char existed");
        size_t i = 0;
        for (; i < num; ++i) {
            if (*tmp == escape_chars[i]) {
                value = values[i];
                break;
            }
        }
        CLOG_RET_IF_X(i == num, NULL, "'\\%c' not supported", *tmp); /* invalid escape char */
    } else {
        value = *tmp;
    }
    tmp++;
    CLOG_RET_IF_X(tmp == end, NULL, "right single quote not found");
    CLOG_RET_IF_X(*tmp != '\'', NULL, "only single character is allowed between single quotation marks");
    item->type = CLOG_CONFIG_TYPE_CHAR;
    item->value.ch = value;
    return tmp + 1;
}

static int32_t clog_config_get_number_value(uint8_t radix, char tmp)
{
    switch (radix) {
        case 2:
            if (tmp == '0' || tmp == '1') {
                return tmp - '0';
            }
            return -1;
        case 8:
            if (tmp >= '0' && tmp <= '7') {
                return tmp - '0';
            }
            return -1;
        case 10:
            if (tmp >= '0' && tmp <= '9') {
                return tmp - '0';
            }
            return -1;
        case 16:
            if (tmp >= '0' && tmp <= '9') {
                return tmp - '0';
            }
            if (tmp >= 'a' && tmp <= 'f') {
                return tmp - 'a' + 10;
            }
            if (tmp >= 'A' && tmp <= 'F') {
                return tmp - 'A' + 10;
            }
            return -1;
        default:
            return -1; /* never go here */
    }
}

static const char* clog_config_parse_uint_radix(const uint8_t radix, const char* start, const char* end,
                                                clog_config_item_t* item)
{
    const char* tmp = start;
    const char* number_end = end;
    uint64_t base = 0;
    while (tmp < end) {
        const int value = clog_config_get_number_value(radix, *tmp);
        if (value < 0) {
            break;
        }
        base = base * radix + value;
        number_end = tmp;
        tmp++;
    }
    CLOG_RET_IF_X(base > UINT32_MAX, NULL, "parsed result %" PRIu64 " overflow", base);
    item->type = CLOG_CONFIG_TYPE_UINT;
    item->value.uint = base;
    return number_end + 1;
}

/* +0.123, -0.123, 0.123 */
static const char* clog_config_parse_double(const char* start, const char* end, clog_config_item_t* item)
{
    bool is_dot = false;
    const char* tmp = start;
    if (*tmp == '+' || *tmp == '-') {
        tmp++;
    }
    const char* number_end = tmp;
    while (tmp < end) {
        if (*tmp == '.') {
            CLOG_RET_IF_X(is_dot, NULL, "too '.' found in double value"); /* only one '.' allowed */
            is_dot = true;
        } else if (*tmp >= '0' || *tmp <= '9') {
            number_end = tmp;
        } else {
            break; /* invalid char */
        }
        tmp++;
    }
    char* str = clog_strndup(start, number_end - start + 1);
    CLOG_RET_IF_NULL_X(str, NULL, "dump double value failed");
    errno = 0;
    const double res = strtod(str, NULL);
    CLOG_CLEAN_RET_IF_X(errno != 0, clog_free(str), NULL, "stood %s failed, err = %d", str, errno);
    clog_free(str);
    item->type = CLOG_CONFIG_TYPE_FLOAT;
    item->value.f = res;
    return number_end + 1;
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

static const char* clog_config_parse_number(const char* start, const char* end, clog_config_item_t* item)
{
    bool is_negative = false;
    bool is_flag_exist = false;
    item->type = CLOG_CONFIG_TYPE_UINT;
    const char* tmp = start;
    const char* number_start = start;
    if (*tmp == '-' || *tmp == '+') {
        is_negative = *tmp == '-';
        is_flag_exist = true;
        tmp++;
        item->type = CLOG_CONFIG_TYPE_INT;
        number_start = tmp;
    }
    if (*tmp == '0') {
        tmp++;
        if ((tmp == end) || (*tmp == ' ') || (*tmp == '#') || (*tmp == '\t')) { /* "+0", "0", "-0" */
            if (item->type == CLOG_CONFIG_TYPE_INT) {
                item->value.sint = 0;
            } else {
                item->value.uint = 0;
            }
            return tmp;
        }
        if (*tmp == '.') {
            tmp--; /* rollback to '0' */
            if (is_flag_exist) {
                tmp--; /* rollback to '+'/'-' */
            }
            return clog_config_parse_double(tmp, end, item);
        }
        const uint8_t radix = clog_config_get_radix(*tmp);
        if (radix != 10) {
            CLOG_RET_IF(is_flag_exist, NULL); /* '+','-' can't be before "0b", "0o", "0x" */
            tmp++;
            return clog_config_parse_uint_radix(radix, tmp, end, item);
        }
        /* other format, e.g. "00", "01.00" */
    }
    CLOG_RET_IF(*tmp == '.', NULL); /* no number before '.', e.g. ".123", "-.0" */
    const char* number_end = NULL;
    while ((*tmp != ' ') && (*tmp != '\t') && (*tmp != '#') && tmp < end) {
        if (*tmp == '.') {
            return clog_config_parse_double(start, end, item); /* parse with '+'/'-' */
        }
        if (*tmp < '0' || *tmp > '9') {
            number_end = tmp;
            break; /* invalid char */
        }
        tmp++;
        number_end = tmp;
    }
    tmp = clog_config_parse_uint_radix(10, number_start, number_end, item);
    CLOG_RET_IF_NULL(tmp, NULL); /* parse without '+'/'-' */
    if (is_negative) { /* negative */
        CLOG_RET_IF_X(item->value.uint > INT32_MAX, NULL, "value -%u overflow", item->value.uint);
        item->value.sint = (int32_t)(item->value.uint * (-1));
    }
    return tmp;
}

static const char* clog_config_parse_string(const char* start, const char* end, clog_config_item_t* item)
{
    const char* tmp = clog_config_remove_leading_spaces(start, end);
    CLOG_RET_IF_X(tmp == end || *tmp != '"', NULL, "value string does not start with '\"'");
    tmp++;

    char* result = clog_malloc(end - start + 1);
    CLOG_RET_IF_NULL_X(result, NULL, "malloc string buffer failed");
    char* out = result;

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
    CLOG_CLEAN_RET_IF_X(!parsed_end, clog_free(result), NULL, "no enclose '\"' found");
    *out = '\0';
    item->value.str = result;
    item->type = CLOG_CONFIG_TYPE_STRING;
    return tmp;
}

static const char* clog_config_parse_bool(const char* start, const char* end, clog_config_item_t* item)
{
    const char* tmp = clog_config_remove_leading_spaces(start, end);
    CLOG_RET_IF_X(tmp >= end, NULL, "value bool is empty");
    if (*tmp == 't') {
        CLOG_RET_IF_X(end - tmp < 4, NULL, "value bool is invalid");
        CLOG_RET_IF_X(strncmp(tmp, "true", 4) != 0, NULL, "value bool starts with 't', but not \"true\"");
        item->value.flag = true;
        item->type = CLOG_CONFIG_TYPE_BOOL;
        return tmp + 4;
    }
    if (*tmp == 'f') {
        CLOG_RET_IF_X(end - tmp < 5, NULL, "value bool is invalid");
        CLOG_RET_IF_X(strncmp(tmp, "false", 5) != 0, NULL, "value bool starts with 'f', but not \"false\"");
        item->value.flag = false;
        item->type = CLOG_CONFIG_TYPE_BOOL;
        return tmp + 5;
    }
    CLOG_ERR_APPEND_LINE("value bool is invalid");
    return NULL;
}

static void clog_config_add_array_item(clog_config_item_t* array, clog_config_item_t* item)
{
    if (array->value.array == NULL) {
        array->value.array = item;
        return;
    }
    clog_config_item_t* child = array->value.array;
    while (child->next != NULL) {
        child = child->next;
    }
    child->next = item;
}

static const char* clog_config_parse_value(const char* start, const char* end, clog_config_item_t* item)
{
    const char* tmp = clog_config_remove_leading_spaces(start, end);
    CLOG_RET_IF_X(tmp == end, NULL, "value is empty");
    if (*tmp == '+' || *tmp == '-' || (*tmp >= '0' && *tmp <= '9')) {
        return clog_config_parse_number(tmp, end, item);
    }
    if (*tmp == '\'') {
        return clog_config_parse_char(tmp, end, item);
    }
    if (*tmp == '"') {
        return clog_config_parse_string(tmp, end, item);
    }
    if (*tmp == 't' || *tmp == 'f') {
        return clog_config_parse_bool(tmp, end, item);
    }
    if (*tmp == '[') {
        item->type = CLOG_CONFIG_TYPE_ARRAY;
        item->value.array = NULL;
        while (tmp < end) {
            tmp = clog_config_remove_leading_spaces(tmp + 1, end);
            CLOG_RET_IF_X(tmp == end, NULL, "value array enclose ']' not found");
            CLOG_RET_IF(*tmp == ']', tmp + 1); /* parse array over */
            clog_config_item_t* child = clog_config_create_empty_item();
            CLOG_RET_IF_NULL_X(child, NULL, "clog_config_create_empty_item failed");
            tmp = clog_config_parse_value(tmp, end, child);
            CLOG_CLEAN_RET_IF_NULL_X(tmp, clog_config_free_item(child), NULL, "parse array item failed");
            clog_config_add_array_item(item, child);
            tmp = clog_config_remove_leading_spaces(tmp, end);
            CLOG_RET_IF_X(tmp == end, NULL, "value array enclose ']' not found");
            CLOG_RET_IF(*tmp == ']', tmp + 1); /* parse array over */
            CLOG_RET_IF_X(*tmp != ',', NULL, "invalid char %c in array", *tmp);
            tmp++;
        }
        CLOG_ERR_APPEND_LINE("value array enclose ']' not found");
    }
    return NULL;
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

/* parse property, e.g. "key = "value"", start is a valid name char */
static clog_config_item_t* clog_config_parse_property(const char* start, const char* end)
{
    enum { PARSE_KEY, PARSE_EQUAL, PARSE_VALUE };
    clog_config_item_t* item = clog_config_create_empty_item();
    CLOG_RET_IF_NULL(item, NULL);
    const char* name_start = start;
    const char* tmp = start + 1;
    uint32_t phase = PARSE_KEY;
    while (tmp < end) {
        if (phase == PARSE_KEY) {
            if (*tmp == ' ' || *tmp == '=' || *tmp == '\t') {
                item->key = clog_strndup(name_start, tmp - name_start);
                CLOG_CLEAN_RET_IF_NULL_X(item->key, clog_config_free_item(item), NULL, "strdup name failed");
                tmp--;
                phase = PARSE_EQUAL;
            }
            if (!clog_config_is_valid_name_char(*tmp)) {
                CLOG_ERR_APPEND_LINE("unexpected character '%c' found in property's name", *tmp);
                clog_config_free_item(item);
                return NULL;
            }
            /* valid name, continue to parse */
        } else if (phase == PARSE_EQUAL) {
            if (*tmp == '=') {
                phase = PARSE_VALUE;
            } else if (*tmp == ' ' || *tmp == '\t') {
                /* continue to parse */
            } else {
                CLOG_ERR_APPEND_LINE("unexpected character '%c' before '='", *tmp);
                clog_config_free_item(item);
                return NULL; /* invalid char */
            }
        } else {
            tmp = clog_config_parse_value(tmp, end, item);
            CLOG_CLEAN_RET_IF_NULL_X(tmp, clog_config_free_item(item), NULL, "parse value of %s failed", item->key);
            CLOG_CLEAN_RET_IF_X(!clog_config_check_inline_comment(tmp, end), clog_config_free_item(item), NULL,
                                "unexcepted character after property %s", item->key);
            return item;
        }
        tmp++;
    }
    CLOG_ERR_APPEND_LINE("invalid property line");
    clog_config_free_item(item);
    return NULL;
}

static clog_config_group_t* clog_config_process_line(const char* start, const char* end, clog_config_group_t* root,
                                                     clog_config_group_t* current)
{
    const char* tmp = clog_config_remove_leading_spaces(start, end);
    CLOG_RET_IF(tmp == end, current); /* empty line */
    if (*tmp == '[') {
        const char* group_start = NULL;
        const char* group_end = NULL;
        tmp = clog_config_get_valid_group_name(tmp, end, &group_start, &group_end);
        CLOG_RET_IF_NULL(tmp, NULL);
        if (!clog_config_check_inline_comment(tmp, end)) {
            return NULL; /* there is invalid char after ']', e.g. "[ test.x ] a" */
        }
        return clog_config_find_or_create_group(root, group_start, group_end);
    }
    if (*tmp == '#') { /* comment line, no need to process */
        return current;
    }
    if (clog_config_is_valid_name_char(*tmp)) {
        clog_config_item_t* item = clog_config_parse_property(tmp, end);
        CLOG_RET_IF_NULL(item, NULL);
        CLOG_RET_IF(clog_config_add_item(current, item), current);
        clog_config_free_item(item);
        return NULL;
    }
    CLOG_ERR_APPEND_LINE("invalid line");
    return NULL;
}

clog_config_group_t* clog_config_parse(const char* data)
{
    CLOG_RET_IF_NULL_X(data, NULL, "data is NULL");
    clog_config_group_t* root = clog_config_create_empty_group();
    clog_config_group_t* current = root; /* current parsing group */
    CLOG_RET_IF_NULL_X(root, NULL, "clog_config_create_empty_group failed");
    const char* tmp = data;
    while (*tmp != '\0') {
        const char* start = tmp; /* start of a line */
        while (*tmp != '\n' && *tmp != '\r' && *tmp != '\0') {
            tmp++;
        }
        const char* end = tmp; /* end of a line, \n or \0 */
        if (start != end) {
            /* start to end indicate a line, end char is not included */
            current = clog_config_process_line(start, end, root, current);
            if (current == NULL) {
                char* line = clog_strndup(start, end - start + 1);
                CLOG_ERR_APPEND_LINE("process line error: %s", line == NULL ? "NULL" : line);
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

static size_t clog_config_dump_set_indent(char* buf, const size_t size, const size_t level, const char* indent)
{
    CLOG_RET_IF_NULL(indent, 0);
    const size_t indent_len = strlen(indent);
    CLOG_RET_IF(indent_len == 0, 0);
    const size_t num = level * indent_len;
    CLOG_RET_IF(num >= size, 0);
    size_t offset = 0;
    for (size_t i = 0; i < level; i++) {
        CLOG_IGNORE_RES(clog_memcpy(buf + offset, size - offset, indent, indent_len));
        offset += indent_len;
    }
    return offset;
}

static size_t clog_config_dump_item(const clog_config_item_t* item, char* buf, const size_t size, const size_t level,
                                    const char* indent)
{
    size_t offset = clog_config_dump_set_indent(buf, size, level, indent);
    CLOG_RET_IF(offset == 0 && level > 0 && indent != NULL, 0);

    int len = 0;
    switch (item->type) {
        case CLOG_CONFIG_TYPE_INT:
            if (item->key == NULL) {
                len = snprintf(buf + offset, size - offset, "%d", item->value.sint);
            } else {
                len = snprintf(buf + offset, size - offset, "%s = %d", item->key, item->value.sint);
            }
            break;
        case CLOG_CONFIG_TYPE_UINT:
            if (item->key == NULL) {
                len = snprintf(buf + offset, size - offset, "%u", item->value.uint);
            } else {
                len = snprintf(buf + offset, size - offset, "%s = %u", item->key, item->value.uint);
            }
            break;
        case CLOG_CONFIG_TYPE_FLOAT:
            if (item->key == NULL) {
                len = snprintf(buf + offset, size - offset, "%lf", item->value.f);
            } else {
                len = snprintf(buf + offset, size - offset, "%s = %lf", item->key, item->value.f);
            }
            break;
        case CLOG_CONFIG_TYPE_CHAR:
            if (item->key == NULL) {
                len = snprintf(buf + offset, size - offset, "%c", item->value.ch);
            } else {
                len = snprintf(buf + offset, size - offset, "%s = %c", item->key, item->value.ch);
            }
            break;
        case CLOG_CONFIG_TYPE_STRING:
            if (item->key == NULL) {
                len = snprintf(buf + offset, size - offset, "\"%s\"", item->value.str);
            } else {
                len = snprintf(buf + offset, size - offset, "%s = \"%s\"", item->key, item->value.str);
            }
            break;
        case CLOG_CONFIG_TYPE_BOOL:
            if (item->key == NULL) {
                len = snprintf(buf + offset, size - offset, "%s", item->value.flag ? "true" : "false");
            } else {
                len = snprintf(buf + offset, size - offset, "%s = %s", item->key, item->value.flag ? "true" : "false");
            }

            break;
        case CLOG_CONFIG_TYPE_ARRAY:
            if (item->key != NULL) {
                len = snprintf(buf + offset, size - offset, "%s = [ ", item->key);
            } else {
                len = snprintf(buf + offset, size - offset, "[ ");
            }
            CLOG_RET_IF(len <= 0, 0);
            offset += len;
            CLOG_RET_IF(offset >= size, 0);
            const clog_config_item_t* child = item->value.array;
            if (child == NULL) {
                buf[offset++] = ']';
                return offset;
            }
            size_t count = 0;
            while (child != NULL) {
                count = clog_config_dump_item(child, buf + offset, size - offset, 0, indent);
                CLOG_RET_IF(count == 0, 0);
                offset += count;
                child = child->next;
                if (child != NULL) {
                    CLOG_RET_IF(offset >= size, 0);
                    buf[offset++] = ',';
                    CLOG_RET_IF(offset >= size, 0);
                    buf[offset++] = ' ';
                }
            }
            CLOG_RET_IF(offset >= size, 0);
            buf[offset++] = ' ';
            CLOG_RET_IF(offset >= size, 0);
            buf[offset++] = ']';
            return offset;
        default:
            return 0;
    }
    CLOG_RET_IF(len <= 0 || (size_t)len >= size - offset, 0);
    return offset + len;
}

const clog_config_group_t* clog_config_find_group(const clog_config_group_t* root, const char* groups[],
                                                  const size_t count)
{
    CLOG_RET_IF_NULL(root, NULL);
    CLOG_RET_IF_NULL(groups, NULL);
    CLOG_RET_IF_NULL(groups[0], NULL);
    CLOG_RET_IF(groups == NULL || count == 0, root);
    const clog_config_group_t* group = root->child;
    while (group != NULL) {
        if (strcmp(group->name, groups[0]) == 0) {
            if (count == 1) {
                return group;
            }
            return clog_config_find_group(group, groups + 1, count - 1);
        }
        group = group->sibling;
    }
    return NULL;
}

/**
 * get item from group
 * @param group group
 * @param key item key, nonnull
 * @return item if success, NULL otherwise
 */
const clog_config_item_t* clog_config_find_item_in_group(const clog_config_group_t* group, const char* key)
{
    CLOG_RET_IF_NULL(group, NULL);
    CLOG_RET_IF_NULL(key, NULL);
    const clog_config_item_t* item = group->content;
    while (item != NULL) {
        if (strcmp(item->key, key) == 0) {
            return item;
        }
        item = item->next;
    }
    return NULL;
}

#define CLOG_DECLARE_FIND_ITEM_FUNC(suffix, t, mem, item_type)                                                      \
    clog_res_e clog_config_find_item_in_group_##suffix(const clog_config_group_t* group, const char* key, t* value) \
    {                                                                                                               \
        CLOG_RET_IF_NULL(value, CLOG_INVALID_PARAM);                                                                \
        const clog_config_item_t* item = clog_config_find_item_in_group(group, key);                                \
        CLOG_RET_IF_NULL(item, CLOG_TARGET_NOT_FOUND);                                                              \
        CLOG_RET_IF(item->type != (item_type), CLOG_ERROR_FORMAT);                                                  \
        *value = item->value.mem;                                                                                   \
        return CLOG_SUCCESS;                                                                                        \
    }

CLOG_DECLARE_FIND_ITEM_FUNC(uint, uint32_t, uint, CLOG_CONFIG_TYPE_UINT)
CLOG_DECLARE_FIND_ITEM_FUNC(bool, bool, flag, CLOG_CONFIG_TYPE_BOOL)
CLOG_DECLARE_FIND_ITEM_FUNC(string, const char*, str, CLOG_CONFIG_TYPE_STRING)
#undef CLOG_DECLARE_FIND_ITEM_FUNC

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
    CLOG_RET_IF_NULL(groups[0], NULL);
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
                                              const size_t level, const char* indent)
{
    size_t offset = clog_config_dump_set_indent(buf, size, level, indent);
    CLOG_RET_IF(offset == 0 && level > 0 && indent != NULL, 0);
    if (group->name != NULL) {
        const size_t len = snprintf(buf + offset, size - offset, "[%s]\n", group->name);
        CLOG_RET_IF(len <= 0 || len >= size - offset, 0);
        offset += len;
    }

    const clog_config_item_t* item = group->content;
    while (item != NULL) {
        const size_t len = clog_config_dump_item(item, buf + offset, size - offset, level + 1, indent);
        CLOG_RET_IF(len == 0, 0);
        offset += len;
        CLOG_RET_IF(offset >= size, 0);
        buf[offset++] = '\n';
        CLOG_RET_IF(offset >= size, 0);
        item = item->next;
    }
    const clog_config_group_t* child = group->child;
    while (child != NULL) {
        const size_t len = clog_config_dump_group_internal(child, buf + offset, size - offset, level + 1, indent);
        CLOG_RET_IF(len == 0, 0);
        offset += len;
        child = child->sibling;
    }
    CLOG_RET_IF(offset >= size, 0);
    buf[offset] = '\0';
    return offset;
}

bool clog_config_dump_group(const clog_config_group_t* group, char* buf, const size_t size, const char* indent)
{
    CLOG_RET_IF_NULL(group, false);
    CLOG_RET_IF_NULL(buf, false);
    return clog_config_dump_group_internal(group, buf, size, 0, indent) != 0;
}
