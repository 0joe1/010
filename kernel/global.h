#ifndef KERNEL_GLOBAL_H
#define KERNEL_GLOBAL_H
#include "stdint.h"

#define DESC_G_4k 1
#define DESC_D_32 1
#define DESC_L 0
#define DESC_AVL 0
#define DESC_P 1      //c里面没有用后面加个b表示二进制的方法
#define DPL_0  0
#define DPL_3  3

#define DESC_S_SYS   0
#define DESC_S_DATA  1
/***********************selector*************************/
#define TI_GDT  0
#define TI_LDT  4
#define SELECTOR_K_CODE  (0x01<<3) + TI_GDT + DPL_0
#define SELECTOR_K_DATA  (0x02<<3) + TI_GDT + DPL_0
#define SELECTOR_K_STACK SELECTOR_K_DATA
#define SELECTOR_U_CODE  (0x05<<3) + TI_GDT + DPL_3
#define SELECTOR_U_DATA  (0x06<<3) + TI_GDT + DPL_3
#define SELECTOR_U_STACK SELECTOR_U_DATA

#define SELECTOR_TSS     (0x04<<3) + TI_GDT + DPL_0

/******************normal descriptor************************/
#define DESC_CODE_TYPE 0x8
#define DESC_DATA_TYPE 0x2

#define USRCODE_ATTR_LOW  ((DESC_P<<7)+(DPL_3<<5)+(DESC_S_DATA<<4)+(DESC_CODE_TYPE))
#define USRCODE_ATTR_HIGH ((DESC_G_4k<<7)+(DESC_D_32<<6)+(DESC_AVL<<4))
#define USRDATA_ATTR_LOW  ((DESC_P<<7)+(DPL_3<<5)+(DESC_S_DATA<<4)+(DESC_DATA_TYPE))
#define USRDATA_ATTR_HIGH ((DESC_G_4k<<7)+(DESC_D_32<<6)+(DESC_AVL<<4))

/******************int descriptor************************/
#define INT_DESC_TYPE 0xe

#define INT_ATTR_DPL0 ((DESC_P<<7)+(DPL_0<<5)+INT_DESC_TYPE)
#define INT_ATTR_DPL3 ((DESC_P<<7)+(DPL_3<<5)+INT_DESC_TYPE)

/**********************tss desc*********************/
#define TSS_DESC_TYPE 0x9
#define TSS_B 2

#define TSS_ATTR_LOW  ((DESC_P<<7)+(DPL_0<<5)+TSS_DESC_TYPE)
#define TSS_ATTR_HIGH ((DESC_G_4k<<7)+(DESC_AVL<<4))

/********************** page ************************/
#define PAGE_P    1
#define PAGE_RW_R 0
#define PAGE_RW_W 2
#define PAGE_US_S 0
#define PAGE_US_U 4

/**********************EFLAGS **********************/
#define EFLAG_MBS (1<<1)
#define EFLAG_IF_0 0
#define EFLAG_IF_1 (1<<9)
#define EFALG_IOPL_0 0

struct gdt_desc {
    uint_16 seg_low_limit;
    uint_16 seg_low_base;
    uint_8  seg_mid_base;
    uint_8  low_attr;
    uint_8  high_base_attr;
    uint_8  seg_high_base;
};

#define DIV_ROUND_UP(divd,divs) ((divd+divs-1)/divs)
#define MAX(a,b) (a>b?a:b)
#endif
