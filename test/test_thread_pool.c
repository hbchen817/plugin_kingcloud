#include "thread_pool.h"
#include <stdio.h>

void taskFunc(void *arg) {
    int num = *(int *)arg;
    printf("thread %ld is working, number = %d\n", thrd_current(), num);
    thrd_sleep(&(struct timespec){.tv_sec=1}, NULL);
}

int main() {
    thread_pool_t * pool = tp_create(3, 10, 100);
    int num[100];
    for (int i = 0; i < 100; ++i) {
        num[i] = i + 100;
        tp_add_task(pool, taskFunc, &num[i]);
    }
    for (int i = 0; i < 30; ++i) {
        thrd_sleep(&(struct timespec){.tv_sec=1}, NULL);
        printf("-- alive %d, busy %d, queue %d\n", tp_alive_number(pool), tp_busy_number(pool), tp_task_size(pool));
    }
    tp_destroy(pool);
    return 0;
}