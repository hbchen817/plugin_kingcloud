#include "timer_manager.h"
#include "map.h"
#include "thread_pool.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/timerfd.h>
#include <threads.h>
#include <unistd.h>

const int MAX_EVENTS = 32;

typedef struct {
    bool           running;
    int            epoll_fd;
    thrd_t         timer_thread;
    char           thread_name[16];
    thread_pool_t *thread_pool;
    struct map    *callbacks;
    mtx_t          mutex;
} timer_manager_impl_t;

typedef struct {
    bool           periodic;
    bool           inepoll;
    bool           autodelete;
    int            timerfd;
    timer_callback cb;
    void          *context;
} timer_param_t;

static int timer_poll_thread(void *args) {
    timer_manager_impl_t *mgr = (timer_manager_impl_t *)args;
    if (mgr->thread_name[0] == '\0') {
        sprintf(mgr->thread_name, "tm_%08X", (uint32_t)(intptr_t)args);
    }
    prctl(PR_SET_NAME, mgr->thread_name);
    struct epoll_event events[MAX_EVENTS];
    while (mgr->running) {
        int n = epoll_wait(mgr->epoll_fd, events, MAX_EVENTS, -1);
        if (n < 0) {
            break;
        }
        for (int i = 0; i < n; i++) {
            int      timer = events[i].data.fd;
            uint64_t howmany;
            read(timer, &howmany, sizeof(howmany));
            mtx_lock(&mgr->mutex);
            intptr_t             key  = timer;
            struct map_iterator *iter = map_find(mgr->callbacks, (void *)key);
            mtx_unlock(&mgr->mutex);
            if (iter == NULL) {
                continue;
            }
            timer_param_t *param = (timer_param_t *)(iter->val);
            timer_callback cb    = param->cb;
            void          *ctx   = param->context;
            if (param->autodelete) {
                mtx_lock(&mgr->mutex);
                map_erase(iter);
                mtx_unlock(&mgr->mutex);
                close(param->timerfd);
                free(param);
            } else if (!param->periodic) {
                epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, param->timerfd, NULL);
                param->inepoll = false;
            }
            if (cb) {
                if (mgr->thread_pool) {
                    tp_add_task(mgr->thread_pool, cb, ctx);
                } else {
                    cb(ctx);
                }
            }
        }
    }
    return 0;
}

timer_manager_t tm_create(int thread_pool_size, const char *name) {
    timer_manager_impl_t *mgr = malloc(sizeof(timer_manager_impl_t));
    if (mgr == NULL) {
        return mgr;
    }
    memset(mgr, 0, sizeof(timer_manager_impl_t));
    if (name != NULL && name[0] != '\0') {
        strncpy(mgr->thread_name, name, sizeof(mgr->thread_name) - 1);
    }
    mgr->running = true;
    if (thread_pool_size > 0) {
        mgr->thread_pool = tp_create(thread_pool_size, thread_pool_size, 256);
    }
    do {
        if (mtx_init(&mgr->mutex, mtx_plain) != thrd_success) {
            break;
        }
        mgr->callbacks = map_int_cptr_create();
        mgr->epoll_fd  = epoll_create1(0);
        if (mgr->epoll_fd < 0) {
            break;
        }
        if (thrd_create(&mgr->timer_thread, timer_poll_thread, mgr) != thrd_success) {
            break;
        }
        return mgr;
    } while (0);
    mtx_destroy(&mgr->mutex);
    if (mgr->callbacks != NULL) {
        map_delete(mgr->callbacks);
    }
    if (mgr->epoll_fd >= 0) {
        close(mgr->epoll_fd);
    }
    if (mgr->thread_pool) {
        tp_destroy(mgr->thread_pool);
    }
    free(mgr);
    return NULL;
}

void tm_destroy(timer_manager_t manager) {
    timer_manager_impl_t *mgr = (timer_manager_impl_t *)manager;
    mgr->running              = false;
    int dummyfd               = tm_timer_create(manager);
    tm_timer_start(manager, dummyfd, NULL, NULL, 1, 0);
    thrd_join(mgr->timer_thread, NULL);
    tm_timer_destroy(manager, dummyfd);
    if (mgr->thread_pool) {
        tp_destroy(mgr->thread_pool);
    }
    for (struct map_iterator *iter = map_first(mgr->callbacks); iter != NULL; iter = map_first(mgr->callbacks)) {
        timer_param_t *param = (timer_param_t *)iter->val;
        epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, param->timerfd, NULL);
        close(param->timerfd);
        free(param);
        map_erase(iter);
    }
    close(mgr->epoll_fd);
    map_delete(mgr->callbacks);
    mtx_destroy(&mgr->mutex);
    free(mgr);
}

int tm_timer_create(timer_manager_t manager) {
    timer_manager_impl_t *mgr   = (timer_manager_impl_t *)manager;
    int                   fd    = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    timer_param_t        *param = malloc(sizeof(timer_param_t));
    memset(param, 0, sizeof(timer_param_t));
    param->timerfd = fd;
    mtx_lock(&mgr->mutex);
    intptr_t key = fd;
    map_insert(mgr->callbacks, (void *)key, param);
    mtx_unlock(&mgr->mutex);
    return fd;
}

void tm_timer_destroy(timer_manager_t manager, int timer) {
    timer_manager_impl_t *mgr = (timer_manager_impl_t *)manager;
    mtx_lock(&mgr->mutex);
    intptr_t             key  = timer;
    struct map_iterator *iter = map_find(mgr->callbacks, (void *)key);
    if (iter != NULL) {
        free(iter->val);
        map_erase(iter);
    }
    mtx_unlock(&mgr->mutex);
    close(timer);
}

int tm_timer_start(timer_manager_t manager, int timer, timer_callback cb, void *context, long expiration, long interval) {
    timer_manager_impl_t *mgr = (timer_manager_impl_t *)manager;
    mtx_lock(&mgr->mutex);
    intptr_t             key  = timer;
    struct map_iterator *iter = map_find(mgr->callbacks, (void *)key);
    mtx_unlock(&mgr->mutex);
    if (iter == NULL) {
        return -1;
    }
    timer_param_t *param = (timer_param_t *)iter->val;
    param->cb            = cb;
    param->context       = context;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += expiration / 1000;
    ts.tv_nsec += (expiration % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    }
    int ret = timerfd_settime(timer, TFD_TIMER_ABSTIME,
                              &(struct itimerspec){
                                  .it_value = ts, .it_interval = {.tv_sec = interval / 1000, .tv_nsec = (interval % 1000) * 1000000L}
    },
                              NULL);
    if (ret == 0) {
        param->periodic = interval != 0;
        ret             = epoll_ctl(mgr->epoll_fd, EPOLL_CTL_ADD, timer, &(struct epoll_event){.events = EPOLLIN, .data = {.fd = timer}});
        param->inepoll  = ret == 0;
    }
    return ret;
}

int tm_timer_stop(timer_manager_t manager, int timer) {
    timer_manager_impl_t *mgr = (timer_manager_impl_t *)manager;
    timerfd_settime(timer, TFD_TIMER_ABSTIME,
                    &(struct itimerspec){
                        .it_value = {.tv_sec = 0, .tv_nsec = 0},
                          .it_interval = {.tv_sec = 0, .tv_nsec = 0}
    },
                    NULL);
    mtx_lock(&mgr->mutex);
    intptr_t             key  = timer;
    struct map_iterator *iter = map_find(mgr->callbacks, (void *)key);
    mtx_unlock(&mgr->mutex);
    if (iter != NULL) {
        timer_param_t *param = (timer_param_t *)iter->val;
        if (param->inepoll) {
            param->inepoll = false;
            epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, timer, NULL);
        }
    }
    return 0;
}

int tm_run_after(timer_manager_t manager, long delay, timer_callback cb, void *context) {
    timer_manager_impl_t *mgr   = (timer_manager_impl_t *)manager;
    int                   fd    = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    timer_param_t        *param = malloc(sizeof(timer_param_t));
    memset(param, 0, sizeof(timer_param_t));
    param->periodic   = false;
    param->autodelete = true;
    param->timerfd    = fd;
    param->cb         = cb;
    param->context    = context;
    mtx_lock(&mgr->mutex);
    intptr_t key = fd;
    map_insert(mgr->callbacks, (void *)key, param);
    mtx_unlock(&mgr->mutex);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += delay / 1000;
    ts.tv_nsec += (delay % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000L;
    }
    int ret = timerfd_settime(fd, TFD_TIMER_ABSTIME,
                              &(struct itimerspec){
                                  .it_value = ts, .it_interval = {.tv_sec = 0, .tv_nsec = 0}
    },
                              NULL);
    if (ret == 0) {
        ret            = epoll_ctl(mgr->epoll_fd, EPOLL_CTL_ADD, fd, &(struct epoll_event){.events = EPOLLIN, .data = {.fd = fd}});
        param->inepoll = ret == 0;
    }
    return ret;
}
