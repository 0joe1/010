#ifndef USERPROG_TSS_H
#define USERPROG_TSS_H
#include "stdint.h"

void update_tss_esp(struct task_struct* pthread);
struct gdt_desc form_gdt_desc(void* desc_addr, \
                              uint_32 limit  , \
                              uint_8 low_attr, \
                              uint_8 high_attr);
void tss_init(void);

#endif 
