#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "yaml.h"
#include "config.h"

static char *type_to_string(enum dp_conf_value_type type) {
    switch (type) {
        case DP_CONF_VALUE_TYPE_NONE:
            return "none";
        case DP_CONF_VALUE_TYPE_SCALAR:
            return "scalar";
        case DP_CONF_VALUE_TYPE_SEQUENCE:
            return "sequence";
        case DP_CONF_VALUE_TYPE_MAPPING:
            return "mapping";
        default:
            return "unknown";
    }
}

/**
 * 填充从根开始的全路径
 * 返回写入的字符串长度，不包含结尾的NULL
 * 如果缓冲区长度不足，则会截断，并返回期望的缓冲区长度，不包含结尾的NULL
 * */
static int fill_path_name(char *buf, int len, const struct dp_conf_node *conf) {
    int key_len;
    int n;

    if (conf->parent) {
        n = fill_path_name(buf, len, conf->parent);

        if (n >= len - 1) {
            // 已经满了，缓冲区不足
            key_len = 0;
        } else {
            // 有空间继续写
            if (conf->parent->value_type == DP_CONF_VALUE_TYPE_SEQUENCE) {
                key_len = snprintf(buf + n, len - n, "[%s]", conf->name);
            } else if (conf->parent->value_type == DP_CONF_VALUE_TYPE_MAPPING) {
                if (conf->parent->parent)
                    key_len = snprintf(buf + n, len - n, ".%s", conf->name);
                else
                    key_len = snprintf(buf + n, len - n, "%s", conf->name);
            } else {
                LOG_ERROR("fill conf path name failed\n");
                key_len = 0;
            }
        }

        return n + key_len;
    } else {
        if (conf->value_type == DP_CONF_VALUE_TYPE_SEQUENCE) {
            return 0;
        } else if (conf->value_type == DP_CONF_VALUE_TYPE_MAPPING) {
            return 0;
        } else {
            // 出错
            LOG_ERROR("fill conf path name failed\n");
            return 0;
        }
    }
}

static struct dp_conf_node *new_conf_node() {
    struct dp_conf_node *conf;
    conf = (struct dp_conf_node *)malloc(sizeof(*conf));
    if (conf)
        memset(conf, '\0', sizeof(*conf));
    else {
        LOG_ERROR("out of memory\n");
        return NULL;
    }
    conf->value_type = DP_CONF_VALUE_TYPE_NONE;
    return conf;
}

static int cpy_yaml_scalar_string(const struct dp_conf_node *conf, char **str, yaml_node_t *node) {
    if (node->data.scalar.length <= 0) {
        char buf[DP_CONF_NODE_NAME_MAX];
        fill_path_name(buf, sizeof(buf), conf);
        LOG_ERROR("invalid scalar len %d for configuration %s\n", node->data.scalar.length, buf);
        return -1;
    }
    *str = (char *)malloc(node->data.scalar.length + 1);
    if (*str == NULL) {
        LOG_ERROR("out of memory\n");
        return -1;
    }
    memcpy(*str, node->data.scalar.value, node->data.scalar.length);
    (*str)[node->data.scalar.length] = '\0';
    return 0;
}

static int parse_yaml_node(struct dp_conf_node *parent, yaml_document_t *doc, yaml_node_t *node) {
    if (node->type == YAML_NO_NODE) {
        LOG_ERROR("invalid yaml no node\n");
        return -1;
    } else if (node->type == YAML_SCALAR_NODE) {
        LOG_ERROR("invalid yaml scalar node\n");
        return -1;
    } else if (node->type == YAML_SEQUENCE_NODE) {
        yaml_node_item_t    *item;
        yaml_node_t         *value;
        struct dp_conf_node **pprev;
        int                 i, n;

        parent->value_type = DP_CONF_VALUE_TYPE_SEQUENCE;
        pprev = &(parent->head);

        for (item = node->data.sequence.items.start, i = 0; item < node->data.sequence.items.top; item ++, i ++) {
            struct dp_conf_node *conf;
            value = yaml_document_get_node(doc, *item);

            conf = new_conf_node();
            if (conf == NULL) {
                return -1;
            }

            conf->parent = parent;
            n = snprintf(NULL, 0, "%d", i);
            conf->name = (char *)malloc(n + 1);
            if (conf->name == NULL) {
                LOG_ERROR("out of memory\n");
                return -1;
            }
            snprintf(conf->name, n + 1, "%d", i);

            *pprev = conf;
            pprev = &(conf->next);

            if (value->type == YAML_SCALAR_NODE) {
                if (cpy_yaml_scalar_string(conf, &(conf->value), value) != 0) {
                    return -1;
                }
                conf->value_type = DP_CONF_VALUE_TYPE_SCALAR;
            } else {
                if (parse_yaml_node(conf, doc, value) != 0) {
                    return -1;
                }
            }
        }
    } else if (node->type == YAML_MAPPING_NODE) {
        yaml_node_pair_t    *pair;
        yaml_node_t         *key;
        yaml_node_t         *value;
        struct dp_conf_node **pprev;

        parent->value_type = DP_CONF_VALUE_TYPE_MAPPING;
        pprev = &(parent->head);

        for (pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair ++) {
            struct dp_conf_node *conf;
            key = yaml_document_get_node(doc, pair->key);
            value = yaml_document_get_node(doc, pair->value);

            if (key->type != YAML_SCALAR_NODE) {
                LOG_ERROR("invalid key node type %d\n", key->type);
                return -1;
            }

            conf = new_conf_node();
            if (conf == NULL) {
                return -1;
            }

            conf->parent = parent;
            // 由于还没有name，传入parent作为错误定位
            if (cpy_yaml_scalar_string(conf->parent, &(conf->name), key) != 0) {
                // 插进去，等待统一释放
                *pprev = conf;
                pprev = &(conf->next);
                return -1;
            }

            // 处理key冲突
            if (conf_node_lookup_child(parent, conf->name)) {
                char buf[DP_CONF_NODE_NAME_MAX];
                int n;
                // 插进去，等待统一释放
                *pprev = conf;
                pprev = &(conf->next);

                n = fill_path_name(buf, sizeof(buf), conf);
                LOG_ERROR("conflict configuration key \"%s\"%s\n", buf, n >= sizeof(buf) - 1 ? "(truncated)" : "");
                return -1;
            }

            *pprev = conf;
            pprev = &(conf->next);

            if (value->type == YAML_SCALAR_NODE) {
                if (cpy_yaml_scalar_string(conf, &(conf->value), value) != 0) {
                    return -1;
                }
                conf->value_type = DP_CONF_VALUE_TYPE_SCALAR;
            } else {
                if (parse_yaml_node(conf, doc, value) != 0) {
                    return -1;
                }
            }
        }
    } else {
        LOG_ERROR("unknown yaml node type %d\n", node->type);
        return -1;
    }
    return 0;
}

static struct dp_conf_node *parse_yaml_doc(yaml_document_t *doc) {
    struct dp_conf_node *conf;
    yaml_node_t         *root;

    root = yaml_document_get_root_node(doc);
    if (root == NULL) {
        LOG_ERROR("empty yaml document\n");
        return NULL;
    }

    conf = new_conf_node();
    if (conf == NULL) {
        return NULL;
    }

    if (parse_yaml_node(conf, doc, root) != 0) {
        conf_free_root(conf);
        return NULL;
    }

    return conf;
}

struct dp_conf_node *conf_parse_file(const char *f) {
    yaml_parser_t       parser;
    yaml_document_t     doc;
    FILE                *file;
    struct dp_conf_node *root = NULL;

    file = fopen(f, "r");
    if (file == NULL) {
        LOG_ERROR("fopen failed, %s\n", strerror(errno));
        goto err_ret;
    }

    if (yaml_parser_initialize(&parser) != 1) {
        LOG_ERROR("yaml_parser_initialize failed\n");
        goto err_parser_ret;
    }

    yaml_parser_set_input_file(&parser, file);

    if (yaml_parser_load(&parser, &doc) != 1) {
        LOG_ERROR("yaml_parser_load failed\n");
        goto err_load_ret;
    }

    root = parse_yaml_doc(&doc);

    yaml_document_delete(&doc);

err_load_ret:
    yaml_parser_delete(&parser);

err_parser_ret:
    fclose(file);

err_ret:
    return root;
}

void conf_free_root(struct dp_conf_node *root) {
    struct dp_conf_node *child;

    conf_remove_node(root);

    child = root->head;
    while (child != NULL) {
        struct dp_conf_node *tmp = child->next;
        conf_free_root(child);
        child = tmp;
    }

    if (root->head)
        conf_free_root(root->head);
    if (root->name)
        free(root->name);
    if (root->value)
        free(root->value);
    free(root);
}

struct dp_conf_node *conf_node_lookup_child(struct dp_conf_node *node, const char *key) {
    struct dp_conf_node *child;

    for (child = node->head; child != NULL; child = child->next) {
        if (child->name && strcmp(child->name, key) == 0) {
            return child;
        }
    }
    return NULL;
}

struct dp_conf_node *conf_get_node(struct dp_conf_node *node, const char *name) {
    char buf[DP_CONF_NODE_NAME_MAX];
    char *key;
    char *next;

    if (strlen(name) >= sizeof(buf)) {
        LOG_ERROR("configuration name too long\n");
        return NULL;
    }

    strcpy(buf, name);

    key = buf;
    do {
        if ((next = strchr(key, '.')) != NULL)
            *(next++) = '\0';
        node = conf_node_lookup_child(node, key);
        key = next;
    } while (next != NULL && node != NULL);

    return node;
}

/**
 * 因为没有填充value和head，还不确定节点类型，因此仅用于内部使用
 * */
static struct dp_conf_node *conf_get_node_or_create(struct dp_conf_node *node, const char *name) {

    struct dp_conf_node *parent;
    char buf[DP_CONF_NODE_NAME_MAX];
    char *key;
    char *next;

    if (strlen(name) >= sizeof(buf)) {
        LOG_ERROR("configuration name too long\n");
        return NULL;
    }

    strcpy(buf, name);

    key = buf;

    parent = node;
    do {
        if ((next = strchr(key, '.')) != NULL)
            *(next++) = '\0';
        if ((node = conf_node_lookup_child(parent, key)) == NULL) {
            if (parent->value_type != DP_CONF_VALUE_TYPE_MAPPING) {
                LOG_ERROR("confituration can not create node in parent node type '%s'\n", type_to_string(parent->value_type));
                return NULL;
            }

            node = new_conf_node();
            if (node == NULL) {
                return NULL;
            }
            node->name = strdup(key);
            if (node->name == NULL) {
                LOG_ERROR("out of memory\n");
                free(node);
                return NULL;
            }
            node->parent = parent;
            node->next = parent->head;
            parent->head = node;
            if (next != NULL) {
                node->value_type = DP_CONF_VALUE_TYPE_MAPPING;
            } else {
                node->value_type = DP_CONF_VALUE_TYPE_NONE;
            }
        }

        key = next;
        parent = node;
    } while (*next != '\0');

    return node;
}

void conf_remove_node(struct dp_conf_node *node) {
    if (node->parent) {
        struct dp_conf_node **pprev = &(node->parent->head);
        while (*pprev != node && *pprev != NULL) {
            pprev = &((*pprev)->next);
        }
        if (*pprev == NULL) {
            char buf[DP_CONF_NODE_NAME_MAX];
            fill_path_name(buf, sizeof(buf), node);
            LOG_ERROR("configuration remove node '%s' failed\n", buf);
            return;
        }
        *pprev = (*pprev)->next;
        node->next = NULL;
        node->parent = NULL;
    }
}

static void _conf_dump(const struct dp_conf_node *node, const char *prefix, char **name, int level) {
    struct dp_conf_node *child;
    char                buf[DP_CONF_NODE_NAME_MAX];
    int                 i;
    int                 n;
    int                 len;
    
    for (child = node->head; child != NULL; child = child->next) {
        name[level] = strdup(child->name);

        if (child->value_type == DP_CONF_VALUE_TYPE_SCALAR) {
            len = 0;
            for (i = 0; i <= level && len < sizeof(buf) - 1; i ++) {
                if (i == 0) {
                    n = snprintf(buf + len, sizeof(buf) - len, "%s", name[i]);
                } else {
                    n = snprintf(buf + len, sizeof(buf) - len, ".%s", name[i]);
                }
                len += n;
            }
            if (prefix) {
                LOG_DEBUG("%s.%s%s = %s\n", prefix, buf, len >= sizeof(buf) - 1 ? "(truncated)" : "", child->value);
            } else {
                LOG_DEBUG("%s%s = %s\n", buf, len >= sizeof(buf) - 1 ? "(truncated)" : "", child->value);
            }
        } else if (child->value_type == DP_CONF_VALUE_TYPE_SEQUENCE || child->value_type == DP_CONF_VALUE_TYPE_MAPPING) {
            _conf_dump(child, prefix, name, level + 1);
        } else {
            LOG_ERROR("invalid configuration node type '%s'\n", type_to_string(child->value_type));
        }

        free(name[level]);
    }
}

void conf_dump(struct dp_conf_node *node, const char *prefix) {
    char *name[DP_CONF_NODE_LEVEL_MAX];

    _conf_dump(node, prefix, name, 0);
}

int conf_get_value(struct dp_conf_node *node, const char *name, char **ptr) {
    node = conf_get_node(node, name);
    if (node && node->value_type == DP_CONF_VALUE_TYPE_SCALAR) {
        *ptr = node->value;
        return 0;
    } else {
        return -1;
    }
}

int conf_get_long(struct dp_conf_node *node, const char *name, long *ptr) {
    char *endptr;
    node = conf_get_node(node, name);
    if (node && node->value_type == DP_CONF_VALUE_TYPE_SCALAR) {
        long l = strtol(node->value, &endptr, 0);
        if (*(node->value) != '\0' && *endptr == '\0') {
            *ptr = l;
            return 0;
        }
    }
    return -1;
}

static int conf_value_is_true(char *val) {
    char *true_value[] = {
        "true",
        "yes",
        "on",
        "1"
    };
    int i;

    for (i = 0; i < sizeof(true_value) / sizeof(true_value[0]); i ++) {
        if (strcasecmp(val, true_value[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int conf_get_bool(struct dp_conf_node *node, const char *name, int *ptr) {
    char *val;
    *ptr = 0;
    if (conf_get_value(node, name, &val) == 0) {
        *ptr = conf_value_is_true(val);
        return 0;
    }
    return -1;
}

int conf_set(struct dp_conf_node *node, const char *name, char *value) {
    char *old_value;

    node = conf_get_node_or_create(node, name);
    if (node == NULL) {
        return -1;
    }

    if (node->value_type != DP_CONF_VALUE_TYPE_NONE && 
            node->value_type != DP_CONF_VALUE_TYPE_SCALAR) {
        LOG_ERROR("configuration can not set value for node type '%s'\n", type_to_string(node->value_type));
        return -1;
    }

    old_value = node->value;
    node->value = strdup(value);
    if (node->value == NULL) {
        LOG_ERROR("out of memory\n");
        if (node->value_type == DP_CONF_VALUE_TYPE_NONE) {
            // 刚刚生成的节点，移除
            conf_free_root(node);
        } else {
            // 已有节点，恢复原值
            node->value = old_value;
        }
        return -1;
    }

    node->value_type = DP_CONF_VALUE_TYPE_SCALAR;
    if (old_value)
        free(old_value);
    return 0;

}

int conf_fill_path(char *buf, int len, const struct dp_conf_node *node) {
    return fill_path_name(buf, len, node);
}