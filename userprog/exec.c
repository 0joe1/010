#include "exec.h"
#include "string.h"
#include "fs.h"
#include "file.h"
#include "stdio-kernel.h"
#include "thread.h"
#include "global.h"
#include "buildin_cmd.h"

static Bool load_phdr(uint_32 fd,Elf_Phdr* phdr)
{
    uint_32 page_cnt;
    int_32 remain_size;
    remain_size = phdr->p_filesz - (PAGESIZE - (phdr->p_vaddr&0x00000fff));
    if (remain_size > 0) {
        page_cnt = DIV_ROUND_UP(remain_size,PAGESIZE) + 1;
    } else {
        page_cnt = 1;
    }

    uint_32 pg_start = phdr->p_vaddr&0xfffff000;
    uint_32 page = pg_start;
    for (uint_32 p = 0 ; p < page_cnt ; p++)
    {
        uint_32* pde = pde_ptr(page);
        uint_32* pte = pte_ptr(page);

        if (!(*pde & PAGE_P) || !(*pte & PAGE_P)) {
            if (get_a_page(PF_USER,page) == NULL) {
                printk("load_phdr: get_a_page failed\n");
                return false;
            }
        }
        page += PAGESIZE;
    }

    sys_lseek(fd,phdr->p_offset,SEEK_SET);
    sys_read(fd,(void*)phdr->p_vaddr,phdr->p_filesz);
    return true;
}


static int_32 load(const char* pathname)
{
    Elf_Ehdr ehdr;
    memset(&ehdr,0,sizeof(ehdr));

    int_32 ret;
    int_32 fd;
    if ((fd = sys_open(pathname,O_RDONLY)) == -1) {
        printk("load: open file failed\n");
        return -1;
    }
    sys_lseek(fd,0,SEEK_SET);
    if (sys_read(fd,&ehdr,sizeof(ehdr)) == -1) {
        printk("load: read file failed\n");
        ret = -1;
        goto done;
    }

    if (memcmp(ehdr.e_ident,"\177ELF",4) || \
        ehdr.e_ident[4] != 1 || \
        ehdr.e_ident[5] != 1 || \
        ehdr.e_ident[6] != 1)
    {
        printk("not good elf file\n");
        ret = -1;
        goto done;
    }

    Elf_Phdr phdr;
    uint_32 phoff = ehdr.e_phoff;
    for (uint_32 ph = 0 ; ph < ehdr.e_phnum ; ph++)
    {
        memset(&phdr,0,sizeof(phdr));
        sys_lseek(fd,phoff,SEEK_SET);
        if (sys_read(fd,&phdr,sizeof(phdr)) == -1) {
            ret = -1;
            goto done;
        }
        if (phdr.p_type == PT_LOAD) {
            load_phdr(fd,&phdr);   //只加载可加载程序段
        }
        phoff += ehdr.e_phentsize;
    }

    ret = ehdr.e_entry;
done:
    sys_close(fd);
    return ret;
}


int_32 sys_execv(const char* pathname,char** argv)
{
    uint_32 argc = 0;
    while (argv[argc]) argc++;

    int_32 entry_point = load(pathname);
    if (entry_point == -1) {
        printk("sys_execv: load failed\n");
        return -1;
    }

    char buf[MAX_PATH_LEN];
    make_abs_path((char*)pathname,buf);
    const char* fname = strrchr(buf,'/')+1;
    struct task_struct* cur = running_thread();
    memset(cur->name,0,20);
    strcpy(cur->name,fname);

    struct intr_stack* intr = (struct intr_stack*) \
                              ((uint_32)cur+PAGESIZE-sizeof(struct intr_stack));
    /* program control */
    intr->eip = (void*)entry_point;
    intr->esp = (void*)(0xc0000000);
    /* argc and argv */
    intr->ebx = (uint_32)argv;
    intr->ecx = argc;

    asm volatile("movl %0,%%esp;jmp int_exit;"::"g"((uint_32)intr):"memory");

    return 0; //never go here
}




