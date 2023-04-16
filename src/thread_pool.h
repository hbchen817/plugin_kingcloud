#pragma once
#include <stdatomic.h>
#include <stdbool.h>
#include <threads.h>

typedef struct task_t {
    void (*func)(void *arg);
    void *arg;
} task_t;

typedef struct thread_t {
    thrd_t tid;
    bool enable;
} thread_t;

typedef struct thread_pool_t {
    task_t * tasks;
    int capacity;
    int size;
    int front;
    int rear;

    thread_t manager;
    thread_t *workers;
    int num_min;
    int num_max;
    atomic_int num_busy;
    int num_live;
    int num_exit;
    mtx_t mtx_pool;
    cnd_t cnd_notfull;
    cnd_t cnd_notempty;
    cnd_t cnd_shutdown;
    bool shutdown;
} thread_pool_t;

thread_pool_t * tp_create(int minimum, int maximum, int size);
void tp_destroy(thread_pool_t *pool);
int tp_add_task(thread_pool_t *pool, void(*func)(void *), void *arg);
int tp_busy_number(const thread_pool_t *pool);
int tp_alive_number(const thread_pool_t *pool);
int tp_task_size(const thread_pool_t *pool);
