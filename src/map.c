#include "map.h"
#include <stdlib.h>
#include <string.h>

static int keycomp_str(const void *k1, const void *k2) {
    return strcmp((const char *)k1, (const char *)k2);
}

static void keycopy_str(void **k1, const void *k2) {
    size_t len = strlen((const char *)k2);
    *k1 = malloc(len + 1);
    strcpy(*k1, k2);
}

static void keyfree_str(void *key) {
    free(key);
}

static void valcopy_cptr(void **v1, const void *v2) {
    *v1 = (void *)v2;
}

static void valfree_cptr(void *val) {}

struct map * map_str_cptr_create() {
    struct map * m = (struct map *)malloc(sizeof(struct map));
    m->root.rb_node = NULL;
    m->key_comp = keycomp_str;
    m->copy_key = keycopy_str;
    m->copy_val = valcopy_cptr;
    m->free_key = keyfree_str;
    m->free_val = valfree_cptr;
    return m;
}

static int keycomp_int(const void *k1, const void *k2) {
    if (k1 < k2) {
        return -1;
    } else if (k1 > k2) {
        return 1;
    } else {
        return 0;
    }
}

static void keycopy_int(void **k1, const void *k2) {
    *k1 = (void *)k2;
}

static void keyfree_int(void *key) {}

struct map * map_int_cptr_create() {
    struct map * m = (struct map *)malloc(sizeof(struct map));
    m->root.rb_node = NULL;
    m->key_comp = keycomp_int;
    m->copy_key = keycopy_int;
    m->copy_val = valcopy_cptr;
    m->free_key = keyfree_int;
    m->free_val = valfree_cptr;
    return m;
}

void map_delete(struct map *m) {
    while (map_first(m) != NULL) {
        map_erase(map_first(m));
    }
    free(m);
}

struct map_iterator * map_find(struct map *m, const void *key) {
    struct rb_node *node = m->root.rb_node;
    while (node) {
        struct map_iterator *iter = container_of(node, struct map_iterator, node);
        int cmp = m->key_comp(key, iter->key);
        if (cmp < 0) {
            node = node->rb_left;
        } else if (cmp > 0) {
            node = node->rb_right;
        } else {
            return iter;
        }
    }
    return NULL;
}

struct map_iterator * map_insert(struct map *m, const void *key, const void *value) {
    struct rb_node **new_node = &m->root.rb_node;
    struct rb_node *parent = NULL;
    while (*new_node) {
        struct map_iterator *iter = container_of(*new_node, struct map_iterator, node);
        int result = m->key_comp(key, iter->key);
        parent = *new_node;
        if (result < 0) {
            new_node = &((*new_node)->rb_left);
        } else if (result > 0) {
            new_node = &((*new_node)->rb_right);
        } else {
            m->copy_val(&iter->val, value);
            return iter;
        }
    }
    struct map_iterator *data = (struct map_iterator *)malloc(sizeof(struct map_iterator));
    data->m = m;
    m->copy_key(&data->key, key);
    m->copy_val(&data->val, value);
    rb_link_node(&data->node, parent, new_node);
    rb_insert_color(&data->node, &m->root);
    return data;
}

void map_erase(struct map_iterator *iter) {
    if (iter != NULL) {
        rb_erase(&iter->node, &iter->m->root);
        iter->m->free_key(iter->key);
        iter->m->free_val(iter->val);
        free(iter);
    }
}

struct map_iterator * map_first(struct map *m) {
    struct rb_node *node = rb_first(&m->root);
    return rb_entry(node, struct map_iterator, node);
}

struct map_iterator * map_prev(struct map_iterator *iter) {
    struct rb_node *prev = rb_prev(&iter->node);
    return rb_entry(prev, struct map_iterator, node);
}

struct map_iterator * map_next(struct map_iterator *iter) {
    struct rb_node *next = rb_next(&iter->node);
    return rb_entry(next, struct map_iterator, node);
}
