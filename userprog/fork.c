#include "fork.h"
#include "pipe.h"
#include "memory.h"
#include "string.h"
#include "process.h"
#include "stdio-kernel.h"
#include "fs.h"
#include "file.h"
#include "debug.h"

extern void int_exit(void);


struct intr_stack* get_intr(struct task_struct* pthread) {
    return (struct intr_stack*)((uint_32)pthread + PAGESIZE - sizeof(struct intr_stack));
}

struct task_struct* running_pcb(void) {
    return (struct task_struct*)running_thread();
}

static int_32 copy_pcb(struct task_struct* parent,struct task_struct* child)
{
    memcpy(child,parent,PAGESIZE);
    child->kstack_p = (uint_32*)((uint_32)child + PAGESIZE);
    fork_pid(child);
    child->ppid     = parent->pid;
    child->ticks    = parent->priority;
    child->elapsed_ticks = 0;
    child->status   = TASK_READY;
    child->all_list_tag.prev = child->all_list_tag.next = NULL;
    child->wait_tag.prev = child->wait_tag.next = NULL;
    block_desc_init(child->usr_block_desc); //没有这个，free_list的head指向parent的tail，有种刻舟求剑的感觉

    uint_32 vaddr_btmp_size = DIV_ROUND_UP((0xc0000000 - USR_VADDR_START)/PAGESIZE/8,PAGESIZE);
    child->usrprog_vaddr.btmp.bits = get_kernel_pages(vaddr_btmp_size);
    if (child->usrprog_vaddr.btmp.bits == NULL) {
        printk("copy_pcb: usrprog_vaddr bitmap.bits malloc failed\n");
        return -1;
    }
    memcpy(child->usrprog_vaddr.btmp.bits,  \
           parent->usrprog_vaddr.btmp.bits, \
           parent->usrprog_vaddr.btmp.map_size);

    strcat(child->name,"_fork");
    return 0;
}

static int_32 copy_body_stack3(struct task_struct* parent,struct task_struct* child,void* page)
{
    uint_32 vaddr_start = child->usrprog_vaddr.vaddr_start;
    struct bitmap* btmp = &child->usrprog_vaddr.btmp;
    uint_32 end_byte = btmp->map_size;
    for (uint_32 b = 0 ; b < end_byte ; b++)
    {
        if (btmp->bits[b] == 0) continue;
        uint_32 bit_idx = 0;
        while (bit_idx < 8)
        {
            if ((btmp->bits[b] & (1<<bit_idx)) == 0) {
                bit_idx++;
                continue;
            }
            uint_32 vaddr = (b*8+bit_idx)*PAGESIZE + vaddr_start;
            memcpy(page,(void*)vaddr,PAGESIZE);
            process_activate(child);
            void* v = get_a_page_without_opvaddrbitmap(PF_KERNEL,vaddr);
            if (v == NULL) {
                printk("copy_body_stack3: get_a_page_without_opvaddrbitmap failed\n");
                return -1;
            }
            memcpy(v,page,PAGESIZE);
            process_activate(parent);
            bit_idx++;
        }
    }
    return 0;
}

static void make_switch_prepare(struct task_struct* child)
{
    struct intr_stack* intr = (struct intr_stack*)\
                              ((uint_32)child->kstack_p-sizeof(struct intr_stack));
    intr->eax = 0;
    // sys_fork 在kernel.S里压的栈保存上下文，跟用户初启动不同，不能随意破坏。

    uint_32* ret_addr = (uint_32*)((uint_32)intr - 1*sizeof(uint_32));
    uint_32* esi_ptr  = (uint_32*)((uint_32)intr - 5*sizeof(uint_32));
    *ret_addr = (uint_32)int_exit;
    child->kstack_p = esi_ptr;
}

static int_32 update_inode_openstat(struct task_struct* child)
{
    for (int_32 fd = 3 ; fd < MAX_OPEN_FILES_PROC ; fd++)
    {
        if (child->fd_table[fd] == 0) continue;
        int_32 _fd = fdlocal2gloabl(fd);
        struct file* f = &file_table[_fd];
        if (f->inode == NULL) {
            return -1;
        }

        if (is_pipe(fd))
            f->fd_pos++;
        else
            f->inode->open_cnts++;
    }
    return 0;
}

static int_32 copy_process(struct task_struct* parent,struct task_struct* child)
{
    void* page = get_kernel_pages(1);
    if (page == NULL) {
        printk("copy_process: page malloc failed\n");
        return -1;
    }

    if (copy_pcb(parent,child) == -1) {
        return -1;
    }

    create_page_dir(child);
    if (copy_body_stack3(parent,child,page) == -1) {
        return -1;
    }

    make_switch_prepare(child);

    update_inode_openstat(child);

    mfree_page(PF_KERNEL,page,1);
    return 0;
}

pid_t sys_fork(void)
{
    struct task_struct* cur = running_thread();
    struct task_struct* child = get_kernel_pages(1);
    if (child == NULL) {
        printk("sys_fork: child get_kernel_pages failed\n");
        return -1;
    }

    if (copy_process(cur,child) == -1) {
        printk("sys_fork: copy_process failed\n");
        return -1;
    }

    ASSERT(!elem_find(&all_thread_list,&child->all_list_tag));
    list_append(&all_thread_list,&child->all_list_tag);
    ASSERT(!elem_find(&thread_ready_list,&child->wait_tag));
    list_append(&thread_ready_list,&child->wait_tag);

    return child->pid;
}
