#include "thread_pool.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>

static const int batch_limit = 2;

static int manager(void *arg);
static int worker(void *arg);

thread_pool_t *tp_create(int minimum, int maximum, int size) {
    if (minimum > maximum) {
        return NULL;
    }
    thread_pool_t *pool = malloc(sizeof(thread_pool_t) + sizeof(thread_t) * maximum + sizeof(task_t) * size);
    if (pool == NULL) {
        return NULL;
    }
    memset(pool, 0, sizeof(thread_pool_t) + sizeof(thread_t) * maximum + sizeof(task_t) * size);
    pool->workers  = (thread_t *)((char *)pool + sizeof(thread_pool_t));
    pool->num_min  = minimum;
    pool->num_max  = maximum;
    pool->num_busy = 0;
    pool->num_live = minimum;
    pool->num_exit = 0;
    if (mtx_init(&pool->mtx_pool, mtx_plain) != thrd_success || cnd_init(&pool->cnd_notempty) != thrd_success || cnd_init(&pool->cnd_notfull) != thrd_success ||
        cnd_init(&pool->cnd_shutdown) != thrd_success) {
        free(pool);
        return NULL;
    }
    pool->tasks    = (task_t *)((char *)pool + sizeof(thread_pool_t) + sizeof(thread_t) * maximum);
    pool->capacity = size;
    pool->size     = 0;
    pool->front    = 0;
    pool->rear     = 0;
    pool->shutdown = false;

    if (minimum < maximum) {
        pool->manager.enable = true;
        thrd_create(&pool->manager.tid, manager, pool);
    }
    for (int i = 0; i < minimum; ++i) {
        pool->workers[i].enable = true;
        thrd_create(&pool->workers[i].tid, worker, pool);
        thrd_detach(pool->workers[i].tid);
    }
    return pool;
}

void tp_destroy(thread_pool_t *pool) {
    if (pool != NULL) {
        mtx_lock(&pool->mtx_pool);
        pool->shutdown = true;
        if (pool->manager.enable) {
            cnd_signal(&pool->cnd_shutdown);
            mtx_unlock(&pool->mtx_pool);
            thrd_join(pool->manager.tid, NULL);
            mtx_lock(&pool->mtx_pool);
        }
        pool->num_exit = pool->num_live;
        cnd_broadcast(&pool->cnd_notempty);
        cnd_wait(&pool->cnd_shutdown, &pool->mtx_pool);
        mtx_unlock(&pool->mtx_pool);
        mtx_destroy(&pool->mtx_pool);
        cnd_destroy(&pool->cnd_notempty);
        cnd_destroy(&pool->cnd_notfull);
        cnd_destroy(&pool->cnd_shutdown);
        free(pool);
    }
}

int tp_add_task(thread_pool_t *pool, void (*func)(void *), void *arg) {
    if (pool == NULL) {
        return -1;
    }
    mtx_lock(&pool->mtx_pool);
    if (pool->size == pool->capacity) {
        mtx_unlock(&pool->mtx_pool);
        return 0;
    }
    pool->tasks[pool->rear].func = func;
    pool->tasks[pool->rear].arg  = arg;
    if (pool->rear >= pool->capacity - 1) {
        pool->rear = 0;
    } else {
        pool->rear++;
    }
    pool->size++;
    cnd_signal(&pool->cnd_notempty);
    mtx_unlock(&pool->mtx_pool);
    return 1;
}

int tp_busy_number(const thread_pool_t *pool) {
    return pool->num_busy;
}

int tp_alive_number(const thread_pool_t *pool) {
    return pool->num_live;
}

int tp_task_size(const thread_pool_t *pool) {
    return pool->size;
}

static int manager(void *arg) {
    char thread_name[16] = {0};
    sprintf(thread_name, "tpmgr_%08X", (uint32_t)(intptr_t)arg);
    prctl(PR_SET_NAME, thread_name);
    thread_pool_t *pool = (thread_pool_t *)arg;
    while (!pool->shutdown) {
        mtx_lock(&pool->mtx_pool);
        int size = pool->size;
        int live = pool->num_live;
        mtx_unlock(&pool->mtx_pool);
        if (size > live && live < pool->num_max) {
            int increase = batch_limit;
            if (increase > size - live) {
                increase = size - live;
            }
            if (increase > pool->num_max - live) {
                increase = pool->num_max - live;
            }
            pool->num_live += increase;
            for (int i = 0, count = 0; i < pool->num_max && count < increase; ++i) {
                if (!pool->workers[i].enable) {
                    pool->workers[i].enable = true;
                    thrd_create(&pool->workers[i].tid, worker, pool);
                    count++;
                }
            }
        } else if (atomic_load(&pool->num_busy) * 2 < live && live > pool->num_min) {
            mtx_lock(&pool->mtx_pool);
            pool->num_exit = live - pool->num_min;
            if (pool->num_exit > batch_limit) {
                pool->num_exit = batch_limit;
            }
            cnd_broadcast(&pool->cnd_notempty);
            mtx_unlock(&pool->mtx_pool);
        }
        struct timespec ts;
        timespec_get(&ts, TIME_UTC);
        ts.tv_sec += 3;
        mtx_lock(&pool->mtx_pool);
        cnd_timedwait(&pool->cnd_shutdown, &pool->mtx_pool, &ts);
        mtx_unlock(&pool->mtx_pool);
    }
    return 0;
}

static int worker(void *arg) {
    thread_pool_t *pool        = (thread_pool_t *)arg;
    thrd_t         current_tid = thrd_current();
    for (int i = 0; i < pool->num_max; i++) {
        if (pool->workers[i].tid == current_tid) {
            char thread_name[16] = {0};
            sprintf(thread_name, "tp_%08X_%d", (uint32_t)(intptr_t)arg, i & 0xFF);
            prctl(PR_SET_NAME, thread_name);
            break;
        }
    }
    while (true) {
        mtx_lock(&pool->mtx_pool);
        if (pool->num_exit > 0) {
            thrd_t tid = thrd_current();
            for (int i = 0; i < pool->num_max; ++i) {
                if (pool->workers[i].enable && thrd_equal(pool->workers[i].tid, tid)) {
                    pool->workers[i].enable = false;
                    pool->num_exit--;
                    pool->num_live--;
                    if (pool->num_live == 0) {
                        cnd_signal(&pool->cnd_shutdown);
                    }
                    mtx_unlock(&pool->mtx_pool);
                    return 0;
                }
            }
        }
        if (pool->size > 0) {
            task_t task = pool->tasks[pool->front];
            if (pool->front + 1 >= pool->capacity) {
                pool->front = 0;
            } else {
                pool->front++;
            }
            pool->size--;
            cnd_signal(&pool->cnd_notfull);
            mtx_unlock(&pool->mtx_pool);
            atomic_fetch_add(&pool->num_busy, 1);
            task.func(task.arg);
            atomic_fetch_sub(&pool->num_busy, 1);
        } else {
            cnd_wait(&pool->cnd_notempty, &pool->mtx_pool);
            mtx_unlock(&pool->mtx_pool);
        }
    }
    return 0;
}