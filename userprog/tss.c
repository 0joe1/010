#include "global.h"
#include "print.h"
#include "thread.h"

struct tss {
    uint_32  last_tss;
    uint_32* esp0;
    uint_32  ss0;
    uint_32* esp1;
    uint_32  ss1;
    uint_32* esp2;
    uint_32  ss2;
    uint_32  cr3;
    uint_32 (*eip)(void);
    uint_32  eflags;
    uint_32  eax;
    uint_32  ecx;
    uint_32  edx;
    uint_32  ebx;
    uint_32  esp;
    uint_32  ebp;
    uint_32  esi;
    uint_32  edi;
    uint_32  es;
    uint_32  cs;
    uint_32  ss;
    uint_32  ds;
    uint_32  fs;
    uint_32  gs;
    uint_32  ldt;
    uint_32  io_pos;
};
static struct tss tss;

void update_tss_esp(struct task_struct* pthread)
{
    tss.esp0 = (uint_32*)((uint_64)(uint_32)pthread + PAGESIZE);
}
 
struct gdt_desc form_gdt_desc(void* desc_addr, \
                              uint_32 limit  , \
                              uint_8 low_attr, \
                              uint_8 high_attr)
{
    uint_32 addr = (uint_32)desc_addr;
    struct gdt_desc gdt;
    gdt.seg_low_limit = limit&0xffff;
    gdt.seg_low_base  = addr&0x0000ffff;
    gdt.seg_mid_base  = (addr&0x00ff0000)>>16;
    gdt.low_attr      = low_attr;
    gdt.high_base_attr= (high_attr | (limit&0xf0000)>>16);
    gdt.seg_high_base = (addr&0xff000000)>>24;

    return gdt;
}

void tss_init(void)
{
    put_str("tss init start\n");
    uint_32 tss_size = sizeof(tss);
    tss.last_tss = 0;
    tss.io_pos = tss_size;
    tss.ss0 = SELECTOR_K_STACK;
    update_tss_esp(running_thread());

    *((struct gdt_desc*)0xc0000620) = form_gdt_desc(&tss, \
                                                    tss_size-1,  \
                                                    TSS_ATTR_LOW, \
                                                    TSS_ATTR_HIGH);

    *((struct gdt_desc*)0xc0000628) = form_gdt_desc((void*)0, \
                                                    0xfffff,  \
                                                    USRCODE_ATTR_LOW, \
                                                    USRCODE_ATTR_HIGH);
    *((struct gdt_desc*)0xc0000630) = form_gdt_desc((void*)0, \
                                                    0xfffff,  \
                                                    USRDATA_ATTR_LOW,
                                                    USRDATA_ATTR_HIGH);
    uint_64 gdt_op = ((7*8-1) | ((uint_64)(uint_32)0xc0000600)<<16);
    asm volatile("lgdt %0"::"m"(gdt_op));
    asm volatile("ltr %w0"::"r"(SELECTOR_TSS));
    put_str("tss init done\n");
}
