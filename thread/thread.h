#ifndef THREAD_H
#define THREAD_H
#include "list.h"
#include "stdint.h"
#include "memory.h"

#define PAGESIZE 4096
#define KMAGIC 0x54878290  //防止栈破坏信息
#define MAX_OPEN_FILES_PROC 20

extern struct list thread_ready_list;
extern struct list all_thread_list;

typedef int_32 pid_t;

typedef void thread_func(void*);

enum task_status{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED,
};


struct intr_stack{
    uint_32 vec_no;
    uint_32 edi;
    uint_32 esi;
    uint_32 ebp;
    uint_32 esp_dump;
    uint_32 ebx;
    uint_32 edx;
    uint_32 ecx;
    uint_32 eax;
    uint_32 gs;
    uint_32 fs;
    uint_32 es;
    uint_32 ds;

    uint_32 err_code;
    void (*eip)(void);
    uint_32 cs;
    uint_32 eflags;
    void* esp;
    uint_32 ss;
};

struct thread_stack {
    uint_32 ebp;
    uint_32 ebx;
    uint_32 edi;
    uint_32 esi;

    void (*eip) (thread_func* ,void* );
    void* unused_retaddr;
    thread_func* func;
    void* arg;
};

struct task_struct {
    uint_32* kstack_p;
    char name[20];
    pid_t pid;
    pid_t ppid;
    struct list_elm wait_tag;
    struct list_elm all_list_tag;
    int_8 exit_status;
    uint_32 ticks;
    uint_32 elapsed_ticks;
    enum task_status status;
    uint_32 priority;
    void* pdir;
    struct virt_addr usrprog_vaddr;
    uint_32 fd_table[MAX_OPEN_FILES_PROC];
    struct mem_block_desc usr_block_desc[DESC_CNT];
    uint_32 cwd_inode_nr;
    uint_32 kmagic;
};

static void asign_pid(struct task_struct* pcb);
struct task_struct* pid2thread(pid_t pid);
void fork_pid(struct task_struct* pcb);
void init_thread(struct task_struct* pcb,char* name,uint_32 priority);
void thread_create(struct task_struct* pcb,thread_func* func,void* arg);
struct task_struct* thread_start(char* name,uint_32 priority,thread_func* func,void* arg);
void* running_thread(void);
void schedule(void);
void make_main_thread(void);
void thread_init(void);
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* pcb);
void idle(void* arg);
void thread_yield(void);
void thread_exit(struct task_struct* exit,Bool need_schedule);
int_32 fdlocal2gloabl(int_32 local_fd);
void pad_print(char* buf,uint_32 bufsize,void* ptr,char format);
Bool elm2thread_info(struct list_elm* elm,int arg);
void sys_ps(void);

#endif
