#ifndef THREAD_SWICH_H
#define THREAD_SWICH_H
#include "thread.h"

void switch_to(struct task_struct* cur,struct task_struct* next);

#endif
