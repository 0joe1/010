#ifndef DEVICE_IOQUEUE_H
#define DEVICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define BUFSIZE 128

struct ioqueue {
    char buf[BUFSIZE];
    struct task_struct* producer;
    struct task_struct* consumer;
    struct lock lock;
    uint_32 head;
    uint_32 tail;
};

void ioq_init(struct ioqueue* ioq);
Bool ioq_full(struct ioqueue* ioq);
Bool ioq_empty(struct ioqueue* ioq);
uint_32 ioq_len(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq,char val);

#endif
