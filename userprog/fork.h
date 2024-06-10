#ifndef USERPROG_FORK_H
#define USERPROG_FORK_H
#include "thread.h"

struct task_struct* running_pcb(void);
struct intr_stack* get_intr(struct task_struct* pthread); //调试用
pid_t sys_fork(void);
static int_32 copy_process(struct task_struct* parent,struct task_struct* child);
static int_32 copy_pcb(struct task_struct* parent,struct task_struct* child);
static int_32 copy_body_stack3(struct task_struct* parent,struct task_struct* child,void* page);
static void make_switch_prepare(struct task_struct* child);
static int_32 update_inode_openstat(struct task_struct* child);

#endif
