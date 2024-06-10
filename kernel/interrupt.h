#ifndef INTERRUPT_H
#define INTERRUPT_H
#include "stdint.h"

#define IDT_DESC_NUMBER 0x81

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

#define EFLAG_IF 0x200
#define GEFLAGS(ef) asm volatile("pushfl ; popl %0":"=g"(ef))

typedef void* gate_addr;
typedef struct {
    uint_16 addr_low16;
    uint_16 selector;
    uint_8  no_use;
    uint_8  attribute;
    uint_16 addr_high_16;
}int_gate_desc;

enum intr_status{
    INTR_OFF,
    INTR_ON
};
enum intr_status get_intr_status();
enum intr_status intr_enable();
enum intr_status intr_disable();
void intr_set_status(enum intr_status);

static int_gate_desc gate_desc_table[IDT_DESC_NUMBER];

void make_idt_desc(int_gate_desc* ,uint_8 ,gate_addr );
void init_pic(void);
void init_idt_desc(void);
void idt_init(void);
void register_handler(uint_8 int_num,gate_addr function);


#endif
