#ifndef DEVICE_CONSOLE_H
#define DEVICE_CONSOLE_H
#include "stdint.h"

void console_init();
void console_put_str(char*);
void console_put_char(uint_8);
void console_put_int(uint_32);


#endif
