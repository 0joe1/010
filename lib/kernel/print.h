#ifndef LIB_KERNEL_PRINT
#define LIB_KERNEL_PRINT
#include "stdint.h"
void put_char(uint_8 c);
void put_str(char* str);
void put_int(uint_32 i);
void set_cursor(uint_16 pos);
extern void cls_screen(void);
#endif
