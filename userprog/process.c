#include "global.h"
#include "debug.h"
#include "process.h"
#include "list.h"
#include "string.h"
#include "interrupt.h"
#include "memory.h"
#include "bitmap.h"
#include "tss.h"

extern void int_exit(void);

void create_page_dir(struct task_struct* pcb)
{
    uint_32* page_dir_vaddr = get_kernel_pages(1);
    memcpy((void*)((uint_32)page_dir_vaddr+0x300*4),(void*)0xfffffc00,255*4);
    page_dir_vaddr[1023] = v2p(page_dir_vaddr)|PAGE_RW_W|PAGE_US_U|PAGE_P;
    pcb->pdir = page_dir_vaddr;
}

void usr_vaddr_init(struct task_struct* pcb)
{
    uint_32 btmp_pgsize = DIV_ROUND_UP((0xc0000000 - USR_VADDR_START)/PAGESIZE/8,PAGESIZE);

    struct virt_addr* usr_vaddr = &pcb->usrprog_vaddr;
    usr_vaddr->vaddr_start = USR_VADDR_START;
    usr_vaddr->btmp.bits     = get_kernel_pages(btmp_pgsize);
    usr_vaddr->btmp.map_size = (0xc0000000-USR_VADDR_START)/PAGESIZE/8;
    bit_init(&pcb->usrprog_vaddr.btmp);
    block_desc_init(pcb->usr_block_desc);
}

void process_execute(char* name,void* filename)
{
    void* pcb_addr = get_kernel_pages(1);
    struct task_struct* pcb = (struct task_struct*)pcb_addr;
    init_thread(pcb,name,DEFALT_PRI);
    create_page_dir(pcb);
    usr_vaddr_init(pcb);
    thread_create(pcb,start_process,filename);

    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&all_thread_list,&pcb->all_list_tag));
    list_append(&all_thread_list,&pcb->all_list_tag);
    ASSERT(!elem_find(&thread_ready_list,&pcb->wait_tag));
    list_append(&thread_ready_list,&pcb->wait_tag);
    intr_set_status(old_status);
}

void start_process(void* filename)
{
    void* func = filename;
    struct task_struct* cur = running_thread();
    cur->kstack_p = (uint_32*)((uint_32)cur->kstack_p + sizeof(struct thread_stack));
    struct intr_stack* intr = (struct intr_stack*)cur->kstack_p;
    intr->vec_no = 0;
    intr->esi = intr->edi = intr->ebp = intr->esp_dump = 0;
    intr->eax = intr->ebx = intr->ecx = intr->edx = 0;
    intr->gs = 0;
    intr->ss = intr->ds = intr->es = intr->gs = SELECTOR_U_DATA;
    intr->cs = SELECTOR_U_CODE;
    intr->eip = func;
    intr->eflags = (EFLAG_MBS | EFALG_IOPL_0 | EFLAG_IF_1);
    intr->esp = (void*)((uint_32)get_a_page(PF_USER,USR_STACK_VADDR)+PAGESIZE);
    asm volatile("movl %0,%%esp;jmp int_exit;"::"g"((uint_32)intr):"memory");
}


void page_dir_activate(struct task_struct* pcb)
{
    uint_32 pdir_paddr;
    if (pcb->pdir == NULL) {
        pdir_paddr = 0x100000;
    } else {
        pdir_paddr = v2p(pcb->pdir);
    }
    asm volatile("movl %0,%%cr3"::"r"(pdir_paddr));
}

void process_activate(struct task_struct* pcb)
{
    page_dir_activate(pcb);
    update_tss_esp(pcb);
}
