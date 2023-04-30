#include "timer_manager.h"
#include <stdio.h>
#include <threads.h>

void func(void *arg) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    char buff[100];
    strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
    printf("Current time: %s.%09ld UTC\n", buff, ts.tv_nsec);
}

int main() {
    // timer_manager_t tm = tm_create(2);
    // int fd = tm_timer_create(tm);
    // tm_timer_start(tm, fd, func, NULL, 5000, 1000);
    // thrd_sleep(&(struct timespec){.tv_sec = 10}, NULL);
    // tm_timer_stop(tm, fd);
    // thrd_sleep(&(struct timespec){.tv_sec = 10}, NULL);
    // tm_destroy(tm);
    timer_manager_t tm = tm_create(0, NULL);
    tm_run_after(tm, 20000, func, NULL);
    thrd_sleep(&(struct timespec){.tv_sec = 1}, NULL);
    tm_destroy(tm);
    return 0;
}