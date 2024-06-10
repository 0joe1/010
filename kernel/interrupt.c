#include "interrupt.h"
#include "stdint.h"
#include "io.h"
#include "global.h"
#include "print.h"

extern gate_addr idt_desc_addr[IDT_DESC_NUMBER];
extern uint_32 syscall_handler(void);
char* int_name[IDT_DESC_NUMBER];
gate_addr idt_table[IDT_DESC_NUMBER];

static void general_intr_handler(uint_8 int_num)
{
    if (int_num == 0x27 || int_num == 0x2f){
        put_str("general_intr_handler");
        return ;
    }
    set_cursor(0);
    put_str("!!!!!!!!!!!   exception message begin !!!!!!!!!!!!!!!\n");
    put_str("int_num:0x");put_int(int_num);put_str("  ");
    put_str(int_name[int_num]);put_str("             ");put_char('\n');
    if (int_num == 14){
        uint_32 vaddr;
        asm volatile("movl %%cr2,%0":"=g"(vaddr));
        //page Fault 异常出现时导致出错的虚拟地址会被加载到cr2中
        put_int(vaddr);put_str(" has no corresponding paddr     \n");
    }
    put_str("!!!!!!!!!!!   exception message end   !!!!!!!!!!!!!!!\n");
    while(1);
    return;
}

void exception_init(void)
{
    for (int i=0;i<IDT_DESC_NUMBER;i++)
    {
        int_name[i] = "unknown";
        idt_table[i] = general_intr_handler;
    }
    int_name[0]="#DE Divide Error";
    int_name[1]="#DB Debug";
    int_name[2]="NMI interrupt";
    int_name[3]="#BP Breakpoint";
    int_name[4]="#OF Overflow";
    int_name[5]="#BR BOUND Range Exceeded";
    int_name[6]="#UD Invalid Opcode";
    int_name[7]="#NM Device not Available";
    int_name[8]="#DF Double Fault";
    int_name[9]="#MF CoProcessor Segment Overun";
    int_name[10]="#TS Invalid TSS";
    int_name[11]="#NP Segment Not Present";
    int_name[12]="#SS Stack Segment Fault";
    int_name[13]="#GP General Protection";
    int_name[14]="#PF Page Fault";
    // int_name[15] Reserved
    int_name[16]="#MF Floating-Point Error(Math Fault)";
    int_name[17]="#AC Alignment Check";
    int_name[18]="#MC Machine Check";
    int_name[19]="#XM SMID Floating-Point Exception";
}


void make_idt_desc(int_gate_desc* gate,uint_8 attr,gate_addr addr)
{
    gate->addr_low16   = (uint_32)addr& 0x0000ffff;
    gate->selector     = SELECTOR_K_CODE;
    gate->no_use       = 0;
    gate->attribute    = attr;
    gate->addr_high_16 = ((uint_32)addr& 0xffff0000)>>16;
}


void init_pic(void)
{
    outb(PIC_M_CTRL,0x11);
    outb(PIC_M_DATA,0x20);
    outb(PIC_M_DATA,0x04);
    outb(PIC_M_DATA,0x01);

    outb(PIC_M_DATA,0xf8);

    outb(PIC_S_CTRL,0x11);
    outb(PIC_S_DATA,0x28);
    outb(PIC_S_DATA,0x02);
    outb(PIC_S_DATA,0x01);

    outb(PIC_S_DATA,0x3f);

    put_str("  pic init done\n");
}

void init_idt_desc(void)
{
    for (int i=0;i<IDT_DESC_NUMBER;i++) {
        make_idt_desc(&gate_desc_table[i],INT_ATTR_DPL0,idt_desc_addr[i]);
    }
    make_idt_desc(&gate_desc_table[0x80],INT_ATTR_DPL3,syscall_handler);
    put_str("  idt init done\n");
}

void idt_init()
{
    init_idt_desc();
    exception_init();
    init_pic();

    uint_64 idt_info = IDT_DESC_NUMBER*8-1;
    idt_info |= ((uint_64)(uint_32)gate_desc_table<<16);
    asm volatile ("lidt %0"::"m"(idt_info));
    put_str("init all done\n");
}

enum intr_status get_intr_status()
{
    uint_32 eflags;
    GEFLAGS(eflags);
    return eflags & EFLAG_IF? INTR_ON : INTR_OFF;
}


enum intr_status intr_enable()
{
    enum intr_status old_status = get_intr_status();
    if (old_status == INTR_OFF){
        asm volatile("sti": : :"cc");
    }
    return old_status;
}

enum intr_status intr_disable()
{
    enum intr_status old_status = get_intr_status();
    if (old_status == INTR_ON){
        asm volatile("cli": : :"cc");
    }
    return old_status;
}

void intr_set_status(enum intr_status status)
{
    status & INTR_ON ?intr_enable():intr_disable();
}

void register_handler(uint_8 int_num,gate_addr function)
{
    idt_table[int_num] = function; 
}
