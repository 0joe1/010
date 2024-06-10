#ifndef _WAIT_EXIT_H
#define _WAIT_EXIT_H
#include "thread.h"

void sys_exit(int_32 status);
pid_t sys_wait(int_32* status);


#endif
