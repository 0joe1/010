#include "thread.h"
#include "string.h"
#include "memory.h"
#include "list.h"
#include "debug.h"
#include "interrupt.h"
#include "switch.h"
#include "print.h"
#include "process.h"
#include "sync.h"
#include "fork.h"
#include "stdio.h"
#include "fs.h"
#include "syscall.h"

#define MAIN_THREAD_PRIO 31
#define MAXPID 1024

extern void init(void);

struct list thread_ready_list;
struct list all_thread_list;

struct task_struct* idle_pcb;
struct task_struct* main_pcb;

char pid_btmp[MAXPID/8];
struct pid_pool {
    struct bitmap btmp;
    pid_t pid_begin;
    struct lock lock;
};

struct pid_pool pid_pool;

void pid_pool_init(void)
{
    pid_pool.pid_begin = 1;
    pid_pool.btmp.map_size = MAXPID/8;
    pid_pool.btmp.bits = (uint_8*)pid_btmp;
    bit_init(&pid_pool.btmp);
    lock_init(&pid_pool.lock);
}

static void asign_pid(struct task_struct* pcb)
{
    lock_acquire(&pid_pool.lock);
    int idx = bit_scan(&pid_pool.btmp,1);
    pcb->pid = pid_pool.pid_begin + idx;
    bitmap_set(&pid_pool.btmp,idx,1);
    lock_release(&pid_pool.lock);
}

static void release_pid(struct task_struct* pcb)
{
    lock_acquire(&pid_pool.lock);
    int idx = pcb->pid - pid_pool.pid_begin;
    bitmap_set(&pid_pool.btmp,idx,0);
    lock_release(&pid_pool.lock);
}

static Bool checkpid(struct list_elm* pelm,pid_t pid)
{
    struct task_struct* pcb = mem2entry(struct task_struct,pelm,all_list_tag);
    if (pcb->pid == pid) {
        return true;
    }
    return false;
}

struct task_struct* pid2thread(pid_t pid)
{
    struct list_elm* elm = list_traversal(&all_thread_list,checkpid,pid);
    if (elm == NULL) {
        return NULL;
    }
    return mem2entry(struct task_struct,elm,all_list_tag);
}

void fork_pid(struct task_struct* pcb){
    asign_pid(pcb);
}

void kernel_thread(thread_func* func,void* arg){
    intr_enable();   //没有iret，EFLAG未恢复，需手动置位IF
    func(arg);
}

void* running_thread()
{
    uint_32 pcb_addr;
    asm volatile("movl %%esp,%0":"=g"(pcb_addr));
    return (void*)((pcb_addr-1) & 0xfffff000);
}
void init_thread(struct task_struct* pcb,char* name,uint_32 priority)
{
    pcb->kstack_p = (void*)((uint_32)pcb + PAGESIZE);
    strcpy((char*)pcb->name,name);
    asign_pid(pcb);
    pcb->ppid = -1;
    pcb->priority = priority;
    pcb->ticks    = priority;
    pcb->elapsed_ticks = 0;
    pcb->status   = TASK_READY;

    for (uint_32 fd = 0 ; fd < MAX_OPEN_FILES_PROC ; fd++) {
        if (fd < 3) pcb->fd_table[fd]=fd;
        else pcb->fd_table[fd] = -1;   //不能是0，0是标准输入
    }
    pcb->cwd_inode_nr = 0;
    pcb->kmagic = KMAGIC;
}

void thread_create(struct task_struct* pcb,thread_func* func,void* arg)
{
    pcb->kstack_p = (uint_32*)((uint_32)pcb->kstack_p - sizeof(struct intr_stack));
    pcb->kstack_p = (uint_32*)((uint_32)pcb->kstack_p - sizeof(struct thread_stack));

    struct thread_stack* thread = (struct thread_stack*)pcb->kstack_p;
    thread->eip  = kernel_thread;
    thread->func = func;
    thread->arg  = arg;
    thread->esi = thread->edi = thread->ebp = thread->ebx = 0;
}

struct task_struct* thread_start(char* name,uint_32 priority,thread_func* func,void* arg)
{
    void* pcb_addr = get_kernel_pages(1);
    struct task_struct* pcb = (struct task_struct*)pcb_addr;
    init_thread(pcb,name,priority);
    thread_create(pcb,func,arg);

    list_append(&all_thread_list,&pcb->all_list_tag);
    list_append(&thread_ready_list,&pcb->wait_tag);
    return pcb_addr;
}

void schedule(void)
{
    ASSERT(get_intr_status() == INTR_OFF)
    struct task_struct* cur = running_thread();
    if (cur->ticks == 0)
    {
        ASSERT(!elem_find(&thread_ready_list,&cur->wait_tag));
        cur->ticks  = cur->priority;
        cur->status = TASK_READY;
        list_append(&thread_ready_list,&cur->wait_tag);
    }

    if (list_empty(&thread_ready_list)) thread_unblock(idle_pcb);
    struct list_elm* next_ready_tag = list_pop(&thread_ready_list);
    struct task_struct* next = mem2entry(struct task_struct,next_ready_tag,wait_tag);
    process_activate(next);
    next->status = TASK_RUNNING;
    switch_to(cur,next);
}

void make_main_thread()
{
    main_pcb = running_thread();
    init_thread(main_pcb,"main_thread",21);
    main_pcb->status = TASK_RUNNING;
    ASSERT(!elem_find(&all_thread_list,&main_pcb->all_list_tag));
    list_append(&all_thread_list,&main_pcb->all_list_tag);
}

void thread_init(void)
{
    put_str("thread init start\n");
    pid_pool_init();
    list_init(&all_thread_list);
    list_init(&thread_ready_list);
    process_execute("init",init);
    make_main_thread();
    idle_pcb = thread_start("idle",10,idle,NULL);
    put_str("thread init done\n");
}

void thread_block(enum task_status stat)
{
    enum intr_status old_status = intr_disable();
    ASSERT(stat==TASK_BLOCKED || \
           stat==TASK_WAITING || \
           stat==TASK_HANGING );

    struct task_struct* cur = (struct task_struct*)running_thread();
    ASSERT(cur->status==TASK_RUNNING);
    cur->status = stat;
    schedule();
    intr_set_status(old_status);
}

void thread_unblock(struct task_struct* thread)
{
    enum intr_status old_status = intr_disable();
    ASSERT(thread->status==TASK_BLOCKED || \
           thread->status==TASK_WAITING || \
           thread->status==TASK_HANGING );

    thread->status = TASK_READY;
    if (elem_find(&thread_ready_list,&thread->wait_tag)) {
        PANIC("thread unblock: already in thread ready list");
    }
    list_push(&thread_ready_list,&thread->wait_tag);
    intr_set_status(old_status);
}

void idle(void* arg)
{
    while (1) {
        thread_block(TASK_BLOCKED);
        asm volatile("sti;hlt;":::"cc");
    }
}


void thread_yield(void)
{
    struct task_struct* cur = running_thread();
    enum intr_status old_status = intr_disable();
    cur->status = TASK_READY;
    list_append(&thread_ready_list,&cur->wait_tag);
    schedule();
    intr_set_status(old_status);
}

void thread_exit(struct task_struct* exit,Bool need_schedule)
{
    intr_disable();
    exit->status = TASK_DIED;
    if (elem_find(&thread_ready_list,&exit->wait_tag))
        list_remove(&exit->wait_tag);
    list_remove(&exit->all_list_tag);

    if (exit->pdir != NULL) {
        mfree_page(PF_KERNEL,exit->pdir,1);
    }

    if (exit != main_pcb) {
        mfree_page(PF_KERNEL,exit,1);
    }

    release_pid(exit);
    if (need_schedule) {
        schedule();
    }
}


int_32 fdlocal2gloabl(int_32 local_fd)
{
    struct task_struct* cur = running_thread();
    int_32 _fd = cur->fd_table[local_fd];
    return _fd;
}

void pad_print(char* buf,uint_32 bufsize,void* ptr,char format)
{
    memset(buf,' ',bufsize);
    buf[bufsize - 1] = '\0';

    switch(format) {
        case 's':
            sprintf(buf,"%s",(char*)ptr);
            if (strlen(buf) != bufsize-1 && buf[strlen(buf)-1] != '\n')
                buf[strlen(buf)] = ' ';
            break;
        case 'd':
            sprintf(buf,"%d",*(uint_32*)ptr);
            break;
        case 'x':
            sprintf(buf,"%x",*(uint_32*)ptr);
            break;
        case 'c':
            sprintf(buf,"%c",*(char*)ptr);
    }
    write(stdout,buf,bufsize);
}

Bool elm2thread_info(struct list_elm* elm,int arg)
{
    struct task_struct* pcb = mem2entry(struct task_struct,elm,all_list_tag);
    char buf[16];
    pad_print(buf,16,&pcb->pid,'d');
    if (pcb->ppid == -1) {
        pad_print(buf,16,"NULL",'s');
    } else {
        pad_print(buf,16,&pcb->ppid,'d');
    }

    pad_print(buf,16,&pcb->ticks,'d');

    switch(pcb->status)
    {
        case TASK_RUNNING:
            pad_print(buf,16,"TASK_RUNNING",'s');
            break;
        case TASK_DIED:
            pad_print(buf,16,"TASK_DIED",'s');
            break;
        case TASK_READY:
            pad_print(buf,16,"TASK_READY",'s');
            break;
        case TASK_BLOCKED:
            pad_print(buf,16,"TASK_BLOCKED",'s');
            break;
        case TASK_HANGING:
            pad_print(buf,16,"TASK_HANGING",'s');
            break;
        case TASK_WAITING:
            pad_print(buf,16,"TASK_WAITING",'s');
            break;
    }

    ASSERT(strlen(pcb->name) < 16);
    char name_buf[16];
    strcpy(name_buf,pcb->name);
    strcat(name_buf,"\n");
    pad_print(buf,16,name_buf,'s');
    return false;
}

void sys_ps(void)
{
    const char* title = "PID             PPID             TICKS             "\
                        "STAT            COMMAND\n";
    sys_write(stdout,title,strlen(title)+1);
    list_traversal(&all_thread_list,elm2thread_info,NULL);
}

