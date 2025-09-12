/**
 * @auther Polaris
 * @date  2025/9/12
 */
#include "clog_config.h"

#include "clog_hooks.h"


typedef struct clog_config_item clog_config_item_t;

struct clog_config_item {
    const char* key;
    const char* value;
    clog_config_item_t* next;
};

typedef struct clog_config_group clog_config_group_t;

struct clog_config_group {
    clog_config_group_t* child;
    clog_config_group_t* sibling;
    clog_config_item_t* content;
};

static inline clog_config_group_t *clog_config_create_group(void)
{
    clog_config_group_t *group = (clog_config_group_t *)clog_malloc(sizeof(clog_config_group_t));
    CLOG_RET_IF_NULL(group, NULL);
    group->child = NULL;
    group->sibling = NULL;
    group->content = NULL;
    return group;
}

static clog_config_group_t *clog_config_parse_raw_data(const char *data)
{
    clog_config_group_t *root = clog_config_create_group();
    clog_config_group_t *current = root; /* current parsing group */
    CLOG_RET_IF_NULL(root, NULL);
    const char *tmp = data;
    while (*tmp != '\0') {
        const char *start = tmp; /* start of a line */
        while ((*tmp != '\n') && (*tmp != '\r') && (*tmp != '\0')) {
            tmp++;
        }
        const char *end = tmp; /* end of a line, \n \r or \0 */
        if (start == end) {
            goto LABLE_NEXT_LINE;
        }

        while (start < end) {
            if (*start == ' ') {
                continue;
            }
            if (*start == '#') { /* comment line, ignore */
                goto LABLE_NEXT_LINE;
            }
            if (*start == '[') {

            }
        }

LABLE_NEXT_LINE:
        while ((*tmp == '\n') || (*tmp == '\r')) {
            tmp++; /* move to start of next line */
        }
    }
    return root;
}

clog_res_e clog_load_config(clog_context_t* ctx, const char* data)
{
    CLOG_RET_IF_NULL(ctx, CLOG_INVALID_PARAM);
    CLOG_RET_IF_NULL(data, CLOG_INVALID_PARAM);

    return CLOG_SUCCESS;
}
