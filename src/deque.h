#pragma once

#include "preprocessor.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#define _QUEUE_TYPE(suffix) BOOST_PP_CAT(queue_, suffix)
#define _QUEUE_ITERATOR(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _iterator)
#define _QUEUE_FUNC_CREATE(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _create)
#define _QUEUE_FUNC_DESTROY(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _destroy)
#define _QUEUE_FUNC_INIT(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _init)
#define _QUEUE_FUNC_DEINIT(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _deinit)
#define _QUEUE_FUNC_EMPTY(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _empty)
#define _QUEUE_FUNC_EMPLACE_FRONT(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _emplace_front)
#define _QUEUE_FUNC_EMPLACE_BACK(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _emplace_back)
#define _QUEUE_FUNC_FRONT(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _front)
#define _QUEUE_FUNC_BACK(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _back)
#define _QUEUE_FUNC_POP_FRONT(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _pop_front)
#define _QUEUE_FUNC_POP_BACK(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _pop_back)
#define _QUEUE_FUNC_MOVE_TO_FRONT(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _move_to_front)
#define _QUEUE_FUNC_MOVE_TO_BACK(suffix) BOOST_PP_CAT(_QUEUE_TYPE(suffix), _move_to_back)
#define _QUEUE_DECLARE_MEMBER(r, data, i, type) type BOOST_PP_CAT(val, i);
#define _QUEUE_DECLARE_FUNC_PARAMS(r, data, i, type) , type BOOST_PP_CAT(v, i)
#define _QUEUE_ASSIGN_VALUS(r, data, i, type) data BOOST_PP_CAT(->val, i) = BOOST_PP_CAT(v, i);

#define DECLARE_QUEUE(suffix, ...)                                                                  \
    typedef struct _QUEUE_ITERATOR(suffix) {                                                        \
        struct _QUEUE_ITERATOR(suffix) *prev;                                                       \
        struct _QUEUE_ITERATOR(suffix) *next;                                                       \
        BOOST_PP_SEQ_FOR_EACH_I(_QUEUE_DECLARE_MEMBER, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))    \
    } _QUEUE_ITERATOR(suffix);                                                                      \
    typedef struct {                                                                                \
        _QUEUE_ITERATOR(suffix) *head;                                                              \
        _QUEUE_ITERATOR(suffix) *tail;                                                              \
        mtx_t mutex;                                                                                \
    } _QUEUE_TYPE(suffix);                                                                          \
    _QUEUE_TYPE(suffix) * _QUEUE_FUNC_CREATE(suffix)();                                             \
    void _QUEUE_FUNC_DESTROY(suffix)(_QUEUE_TYPE(suffix) *q);                                       \
    void _QUEUE_FUNC_INIT(suffix)(_QUEUE_TYPE(suffix) *q);                                          \
    void _QUEUE_FUNC_DEINIT(suffix)(_QUEUE_TYPE(suffix) *q);                                        \
    bool _QUEUE_FUNC_EMPTY(suffix)(const _QUEUE_TYPE(suffix) *q);                                   \
    _QUEUE_ITERATOR(suffix) * _QUEUE_FUNC_EMPLACE_FRONT(suffix)(_QUEUE_TYPE(suffix) *q BOOST_PP_SEQ_FOR_EACH_I(_QUEUE_DECLARE_FUNC_PARAMS, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));  \
    _QUEUE_ITERATOR(suffix) * _QUEUE_FUNC_EMPLACE_BACK(suffix)(_QUEUE_TYPE(suffix) *q BOOST_PP_SEQ_FOR_EACH_I(_QUEUE_DECLARE_FUNC_PARAMS, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));   \
    _QUEUE_ITERATOR(suffix) * _QUEUE_FUNC_FRONT(suffix)(_QUEUE_TYPE(suffix) * q);                   \
    _QUEUE_ITERATOR(suffix) * _QUEUE_FUNC_BACK(suffix)(_QUEUE_TYPE(suffix) * q);                    \
    void _QUEUE_FUNC_POP_FRONT(suffix)(_QUEUE_TYPE(suffix) * q);                                    \
    void _QUEUE_FUNC_POP_BACK(suffix)(_QUEUE_TYPE(suffix) * q);                                     \
    void _QUEUE_FUNC_MOVE_TO_FRONT(suffix)(_QUEUE_TYPE(suffix) * q, _QUEUE_ITERATOR(suffix) * iter, _QUEUE_TYPE(suffix) * another); \
    void _QUEUE_FUNC_MOVE_TO_BACK(suffix)(_QUEUE_TYPE(suffix) * q, _QUEUE_ITERATOR(suffix) * iter, _QUEUE_TYPE(suffix) * another);  \
    /**/

#define IMPLEMENT_QUEUE(suffix, ...)                                                                \
    _QUEUE_TYPE(suffix) * _QUEUE_FUNC_CREATE(suffix)() {                                            \
        _QUEUE_TYPE(suffix) * q = malloc(sizeof(_QUEUE_TYPE(suffix)));                              \
        _QUEUE_FUNC_INIT(suffix)(q);                                                                \
        return q;                                                                                   \
    }                                                                                               \
    void _QUEUE_FUNC_INIT(suffix)(_QUEUE_TYPE(suffix) *q) {                                         \
        memset(q, 0, sizeof(_QUEUE_TYPE(suffix)));                                                  \
        mtx_init(&q->mutex, mtx_plain);                                                             \
    }                                                                                               \
    void _QUEUE_FUNC_DESTROY(suffix)(_QUEUE_TYPE(suffix) *q) {                                      \
        _QUEUE_FUNC_DEINIT(suffix)(q);                                                              \
        free(q);                                                                                    \
    }                                                                                               \
    void _QUEUE_FUNC_DEINIT(suffix)(_QUEUE_TYPE(suffix) *q) {                                       \
        mtx_destroy(&q->mutex);                                                                     \
    }                                                                                               \
    bool _QUEUE_FUNC_EMPTY(suffix)(const _QUEUE_TYPE(suffix) *q) {                                  \
        return q->head == NULL;                                                                     \
    }                                                                                               \
    _QUEUE_ITERATOR(suffix) * _QUEUE_FUNC_EMPLACE_FRONT(suffix)(_QUEUE_TYPE(suffix) *q BOOST_PP_SEQ_FOR_EACH_I(_QUEUE_DECLARE_FUNC_PARAMS, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))) { \
        _QUEUE_ITERATOR(suffix) * iter = malloc(sizeof(_QUEUE_ITERATOR(suffix)));                   \
        memset(iter, 0, sizeof(_QUEUE_ITERATOR(suffix)));                                           \
        BOOST_PP_SEQ_FOR_EACH_I(_QUEUE_ASSIGN_VALUS, iter, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))   \
        mtx_lock(&q->mutex);                                                                        \
        iter->next = q->head;                                                                       \
        q->head = iter;                                                                             \
        if (q->tail == NULL) {                                                                      \
            q->tail = iter;                                                                         \
        }                                                                                           \
        mtx_unlock(&q->mutex);                                                                      \
        return iter;                                                                                \
    }                                                                                               \
    _QUEUE_ITERATOR(suffix) * _QUEUE_FUNC_EMPLACE_BACK(suffix)(_QUEUE_TYPE(suffix) *q BOOST_PP_SEQ_FOR_EACH_I(_QUEUE_DECLARE_FUNC_PARAMS, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))) {  \
        _QUEUE_ITERATOR(suffix) * iter = malloc(sizeof(_QUEUE_ITERATOR(suffix)));                   \
        memset(iter, 0, sizeof(_QUEUE_ITERATOR(suffix)));                                           \
        BOOST_PP_SEQ_FOR_EACH_I(_QUEUE_ASSIGN_VALUS, iter, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))   \
        mtx_lock(&q->mutex);                                                                        \
        iter->prev = q->tail;                                                                       \
        if (q->tail != NULL) {                                                                      \
            q->tail->next = iter;                                                                   \
        } else {                                                                                    \
            q->head = iter;                                                                         \
        }                                                                                           \
        q->tail = iter;                                                                             \
        mtx_unlock(&q->mutex);                                                                      \
        return iter;                                                                                \
    }                                                                                               \
    _QUEUE_ITERATOR(suffix) * _QUEUE_FUNC_FRONT(suffix)(_QUEUE_TYPE(suffix) * q) {                  \
        return q->head;                                                                             \
    }                                                                                               \
    _QUEUE_ITERATOR(suffix) * _QUEUE_FUNC_BACK(suffix)(_QUEUE_TYPE(suffix) * q) {                   \
        return q->tail;                                                                             \
    }                                                                                               \
    void _QUEUE_FUNC_POP_FRONT(suffix)(_QUEUE_TYPE(suffix) * q) {                                   \
        if (q->head == NULL) {                                                                      \
            return;                                                                                 \
        }                                                                                           \
        mtx_lock(&q->mutex);                                                                        \
        _QUEUE_ITERATOR(suffix) * iter = q->head;                                                   \
        q->head = iter->next;                                                                       \
        if (q->head == NULL) {                                                                      \
            q->tail = NULL;                                                                         \
        }                                                                                           \
        mtx_unlock(&q->mutex);                                                                      \
        free(iter);                                                                                 \
    }                                                                                               \
    void _QUEUE_FUNC_POP_BACK(suffix)(_QUEUE_TYPE(suffix) * q) {                                    \
        if (q->tail == NULL) {                                                                      \
            return;                                                                                 \
        }                                                                                           \
        mtx_lock(&q->mutex);                                                                        \
        _QUEUE_ITERATOR(suffix) * iter = q->tail;                                                   \
        q->tail = iter->prev;                                                                       \
        if (q->tail == NULL) {                                                                      \
            q->head = NULL;                                                                         \
        }                                                                                           \
        mtx_unlock(&q->mutex);                                                                      \
        free(iter);                                                                                 \
    }                                                                                               \
    void _QUEUE_FUNC_MOVE_TO_FRONT(suffix)(_QUEUE_TYPE(suffix) * q, _QUEUE_ITERATOR(suffix) * iter, _QUEUE_TYPE(suffix) * another) {    \
        mtx_lock(&q->mutex);                                                                        \
        mtx_lock(&another->mutex);                                                                  \
        if (iter->prev == NULL) {                                                                   \
            q->head = iter->next;                                                                   \
        } else {                                                                                    \
            iter->prev->next = iter->next;                                                          \
        }                                                                                           \
        if (iter->next == NULL) {                                                                   \
            q->tail = iter->prev;                                                                   \
        } else {                                                                                    \
            iter->next->prev = iter->prev;                                                          \
        }                                                                                           \
        iter->prev = NULL;                                                                          \
        iter->next = another->head;                                                                 \
        another->head = iter;                                                                       \
        if (another->tail == NULL) {                                                                \
            another->tail = iter;                                                                   \
        }                                                                                           \
        mtx_unlock(&q->mutex);                                                                      \
        mtx_unlock(&another->mutex);                                                                \
    }                                                                                               \
    void _QUEUE_FUNC_MOVE_TO_BACK(suffix)(_QUEUE_TYPE(suffix) * q, _QUEUE_ITERATOR(suffix) * iter, _QUEUE_TYPE(suffix) * another) {     \
        mtx_lock(&q->mutex);                                                                        \
        mtx_lock(&another->mutex);                                                                  \
        if (iter->prev == NULL) {                                                                   \
            q->head = iter->next;                                                                   \
        } else {                                                                                    \
            iter->prev->next = iter->next;                                                          \
        }                                                                                           \
        if (iter->next == NULL) {                                                                   \
            q->tail = iter->prev;                                                                   \
        } else {                                                                                    \
            iter->next->prev = iter->prev;                                                          \
        }                                                                                           \
        iter->next = NULL;                                                                          \
        iter->prev = another->tail;                                                                 \
        another->tail = iter;                                                                       \
        if (another->head == NULL) {                                                                \
            another->head = iter;                                                                   \
        }                                                                                           \
        mtx_unlock(&q->mutex);                                                                      \
        mtx_unlock(&another->mutex);                                                                \
    }
