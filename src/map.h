#pragma once
#include "preprocessor.h"
#include "rbtree.h"
#include <stdlib.h>
#include <string.h>

typedef int (*key_compare_f)(const void *k1, const void *k2);
typedef void (*copy_key_f)(void **k1, const void *k2);
typedef void (*copy_val_f)(void **v1, const void *v2);
typedef void (*free_key_f)(void *key);
typedef void (*free_val_f)(void *val);

struct map {
    struct rb_root root;
    key_compare_f  key_comp;
    copy_key_f     copy_key;
    copy_val_f     copy_val;
    free_key_f     free_key;
    free_val_f     free_val;
};

struct map_iterator {
    struct rb_node node;
    struct map    *m;
    void          *key;
    void          *val;
};

struct map *map_str_cptr_create();
struct map *map_int_cptr_create();

void                 map_delete(struct map *m);
struct map_iterator *map_find(struct map *m, const void *key);
struct map_iterator *map_insert(struct map *m, const void *key, const void *value);
void                 map_erase(struct map_iterator *iter);
struct map_iterator *map_first(struct map *m);
struct map_iterator *map_prev(struct map_iterator *iter);
struct map_iterator *map_next(struct map_iterator *iter);

#define _MAP_DECLARE_MEMBER(r, data, i, type)      type BOOST_PP_CAT(val, i);
#define _MAP_DECLARE_FUNC_PARAMS(r, data, i, type) , type BOOST_PP_CAT(v, i)
#define _MAP_ASSIGN_VALUS(r, data, i, type)        data BOOST_PP_CAT(->val, i) = BOOST_PP_CAT(v, i);
#define _MAP_TYPE(suffix)                          BOOST_PP_CAT(map_, suffix)
#define _MAP_ITERATOR_TYPE(suffix)                 BOOST_PP_CAT(_MAP_TYPE(suffix), _iterator)
#define _MAP_FUNC_CREATE(suffix)                   BOOST_PP_CAT(_MAP_TYPE(suffix), _create)
#define _MAP_FUNC_DESTROY(suffix)                  BOOST_PP_CAT(_MAP_TYPE(suffix), _destroy)
#define _MAP_FUNC_FIND(suffix)                     BOOST_PP_CAT(_MAP_TYPE(suffix), _find)
#define _MAP_FUNC_INSERT(suffix)                   BOOST_PP_CAT(_MAP_TYPE(suffix), _insert)
#define _MAP_FUNC_FIRST(suffix)                    BOOST_PP_CAT(_MAP_TYPE(suffix), _first)
#define _MAP_FUNC_PREV(suffix)                     BOOST_PP_CAT(_MAP_TYPE(suffix), _prev)
#define _MAP_FUNC_NEXT(suffix)                     BOOST_PP_CAT(_MAP_TYPE(suffix), _next)
#define _MAP_FUNC_ERASE(suffix)                    BOOST_PP_CAT(_MAP_TYPE(suffix), _erase)
#define _MAP_COMPARE_ARITHMATIC(k1, k2)            k1 - k2
#define _MAP_COMPARE_STRING(s1, s2)                strcmp(s1, s2)
#define _MAP_COPY_ASSIGNMENT(lvalue, rvalue)       lvalue = rvalue
#define _MAP_COPY_STRING(s1, s2)                   s1 = strdup(s2)
#define _MAP_FREE_PLAIN(key)
#define _MAP_FREE_STRING(key) free(key)

#define DECLARE_MAP(suffix, key_type, ...)                                                                                                                     \
    typedef struct BOOST_PP_CAT(_, _MAP_TYPE(suffix)) {                                                                                                        \
        struct rb_root root;                                                                                                                                   \
    } _MAP_TYPE(suffix);                                                                                                                                       \
    typedef struct {                                                                                                                                           \
        struct rb_node node;                                                                                                                                   \
        BOOST_PP_CAT(map_, suffix) * m;                                                                                                                        \
        key_type key;                                                                                                                                          \
        BOOST_PP_SEQ_FOR_EACH_I(_MAP_DECLARE_MEMBER, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                                                 \
    } _MAP_ITERATOR_TYPE(suffix);                                                                                                                              \
    _MAP_TYPE(suffix) * _MAP_FUNC_CREATE(suffix)();                                                                                                            \
    void _MAP_FUNC_DESTROY(suffix)(_MAP_TYPE(suffix) * m);                                                                                                     \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_FIND(suffix)(const _MAP_TYPE(suffix) * m, const key_type k);                                                        \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_INSERT(suffix)(_MAP_TYPE(suffix) * m, const key_type k BOOST_PP_SEQ_FOR_EACH_I(                                     \
                                                                                     _MAP_DECLARE_FUNC_PARAMS, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));     \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_FIRST(suffix)(_MAP_TYPE(suffix) * m);                                                                               \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_PREV(suffix)(_MAP_ITERATOR_TYPE(suffix) * iter);                                                                    \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_NEXT(suffix)(_MAP_ITERATOR_TYPE(suffix) * iter);                                                                    \
    void _MAP_FUNC_ERASE(suffix)(_MAP_ITERATOR_TYPE(suffix) * iter);

#define IMPLEMENT_MAP(suffix, COMPARE, COPY, FREE, key_type, ...)                                                                                              \
    _MAP_TYPE(suffix) * _MAP_FUNC_CREATE(suffix)() {                                                                                                           \
        _MAP_TYPE(suffix) *m = (_MAP_TYPE(suffix) *)malloc(sizeof(_MAP_TYPE(suffix)));                                                                         \
        m->root.rb_node      = NULL;                                                                                                                           \
        return m;                                                                                                                                              \
    }                                                                                                                                                          \
    void _MAP_FUNC_DESTROY(suffix)(_MAP_TYPE(suffix) * m) {                                                                                                    \
        while (_MAP_FUNC_FIRST(suffix)(m) != NULL) {                                                                                                           \
            _MAP_FUNC_ERASE(suffix)(_MAP_FUNC_FIRST(suffix)(m));                                                                                               \
        }                                                                                                                                                      \
        free(m);                                                                                                                                               \
    }                                                                                                                                                          \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_FIND(suffix)(const _MAP_TYPE(suffix) * m, const key_type k) {                                                       \
        struct rb_node *node = m->root.rb_node;                                                                                                                \
        while (node) {                                                                                                                                         \
            _MAP_ITERATOR_TYPE(suffix) *iter = container_of(node, _MAP_ITERATOR_TYPE(suffix), node);                                                           \
            int cmp                          = COMPARE(k, iter->key);                                                                                          \
            if (cmp < 0) {                                                                                                                                     \
                node = node->rb_left;                                                                                                                          \
            } else if (cmp > 0) {                                                                                                                              \
                node = node->rb_right;                                                                                                                         \
            } else {                                                                                                                                           \
                return iter;                                                                                                                                   \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        return NULL;                                                                                                                                           \
    }                                                                                                                                                          \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_INSERT(suffix)(_MAP_TYPE(suffix) * m, const key_type k BOOST_PP_SEQ_FOR_EACH_I(                                     \
                                                                                     _MAP_DECLARE_FUNC_PARAMS, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))) {    \
        struct rb_node **new_node = &m->root.rb_node;                                                                                                          \
        struct rb_node  *parent   = NULL;                                                                                                                      \
        while (*new_node) {                                                                                                                                    \
            _MAP_ITERATOR_TYPE(suffix) *iter = container_of(*new_node, _MAP_ITERATOR_TYPE(suffix), node);                                                      \
            int result                       = COMPARE(k, iter->key);                                                                                          \
            parent                           = *new_node;                                                                                                      \
            if (result < 0) {                                                                                                                                  \
                new_node = &((*new_node)->rb_left);                                                                                                            \
            } else if (result > 0) {                                                                                                                           \
                new_node = &((*new_node)->rb_right);                                                                                                           \
            } else {                                                                                                                                           \
                BOOST_PP_SEQ_FOR_EACH_I(_MAP_ASSIGN_VALUS, iter, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                                        \
                return iter;                                                                                                                                   \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        _MAP_ITERATOR_TYPE(suffix) *data = (_MAP_ITERATOR_TYPE(suffix) *)malloc(sizeof(_MAP_ITERATOR_TYPE(suffix)));                                           \
        data->m                          = m;                                                                                                                  \
        COPY(data->key, k);                                                                                                                                    \
        BOOST_PP_SEQ_FOR_EACH_I(_MAP_ASSIGN_VALUS, data, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))                                                                \
        rb_link_node(&data->node, parent, new_node);                                                                                                           \
        rb_insert_color(&data->node, &m->root);                                                                                                                \
        return data;                                                                                                                                           \
    }                                                                                                                                                          \
    void _MAP_FUNC_ERASE(suffix)(_MAP_ITERATOR_TYPE(suffix) * iter) {                                                                                          \
        if (iter != NULL) {                                                                                                                                    \
            rb_erase(&iter->node, &iter->m->root);                                                                                                             \
            FREE(iter->key);                                                                                                                                   \
            free(iter);                                                                                                                                        \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_FIRST(suffix)(_MAP_TYPE(suffix) * m) {                                                                              \
        struct rb_node *node = rb_first(&m->root);                                                                                                             \
        return rb_entry(node, _MAP_ITERATOR_TYPE(suffix), node);                                                                                               \
    }                                                                                                                                                          \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_PREV(suffix)(_MAP_ITERATOR_TYPE(suffix) * iter) {                                                                   \
        struct rb_node *prev = rb_prev(&iter->node);                                                                                                           \
        return rb_entry(prev, _MAP_ITERATOR_TYPE(suffix), node);                                                                                               \
    }                                                                                                                                                          \
    _MAP_ITERATOR_TYPE(suffix) * _MAP_FUNC_NEXT(suffix)(_MAP_ITERATOR_TYPE(suffix) * iter) {                                                                   \
        struct rb_node *next = rb_next(&iter->node);                                                                                                           \
        return rb_entry(next, _MAP_ITERATOR_TYPE(suffix), node);                                                                                               \
    }
