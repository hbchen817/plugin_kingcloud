#pragma once

typedef void *timer_manager_t;

// return non-zero to stop the timer
typedef void (*timer_callback)(void *);

// thread_pool_size: 0 means dont use thread pool
timer_manager_t tm_create(int thread_pool_size, const char *name);

void tm_destroy(timer_manager_t manager);
int  tm_timer_create(timer_manager_t manager);
void tm_timer_destroy(timer_manager_t manager, int timer);
int  tm_timer_start(timer_manager_t manager, int timer, timer_callback cb, void *context, long expiration, long interval);
int  tm_timer_stop(timer_manager_t manager, int timer);
int  tm_run_after(timer_manager_t manager, long delay, timer_callback cb, void *context);
