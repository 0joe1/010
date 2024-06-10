#include "ioqueue.h"
#include "string.h"
#include "debug.h"
#include "interrupt.h"

void ioq_init(struct ioqueue* ioq)
{
    ioq->consumer = ioq->producer = NULL;
    ioq->head = ioq->tail = 0;
    lock_init(&ioq->lock);
}

static uint_32 next_pos(uint_32 pos) {
    return (pos+1)%BUFSIZE;
}

Bool ioq_empty(struct ioqueue* ioq) {
    return (ioq->head == ioq->tail);
}
Bool ioq_full(struct ioqueue* ioq) {
    return (next_pos(ioq->head) == ioq->tail);
}

uint_32 ioq_len(struct ioqueue* ioq) {
    if (ioq->head >= ioq->tail)
        return ioq->head-ioq->tail;
    else
        return (BUFSIZE - (ioq->head - ioq->tail));
}

static void ioq_wait(struct task_struct** waiter)
{
    ASSERT(waiter!=NULL);
    *waiter = running_thread();
    thread_block(TASK_BLOCKED);
}
static void wakeup(struct task_struct** waiter)
{
    ASSERT(waiter!=NULL && *waiter!=NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}

char ioq_getchar(struct ioqueue* ioq)
{
    ASSERT(get_intr_status() == INTR_OFF);
    while (ioq_empty(ioq)){
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }

    char ret_char = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);
    if (ioq->producer) {
        wakeup(&ioq->producer);
    }
    return ret_char;
}

void ioq_putchar(struct ioqueue* ioq,char val)
{
    ASSERT(get_intr_status() == INTR_OFF);
    while (ioq_full(ioq)){
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock);
    }

    ioq->buf[ioq->head] = val;
    ioq->head = next_pos(ioq->head);
    if (ioq->consumer) {
        wakeup(&ioq->consumer);
    }
}

