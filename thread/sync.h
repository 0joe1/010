#ifndef THREAD_SYNC_H
#define THREAD_SYNC_H
#include "stdint.h"
#include "list.h"

struct semaphore {
    uint_8 value;
    struct list wait_list;
};

struct lock {
    struct task_struct* holder;
    struct semaphore sema;
    uint_32 acquire_nr;
};

void sema_init(struct semaphore*,uint_8);
void sema_down(struct semaphore*);
void sema_up(struct semaphore*);
void lock_init(struct lock*);
void lock_acquire(struct lock*);
void lock_release(struct lock*);

#endif
