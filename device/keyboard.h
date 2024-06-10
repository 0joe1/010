#ifndef DEVICE_KEYBOARD_H
#define DEVICE_KEYBOARD_H
#include "stdint.h"

#define esc '\033'
#define backspace '\b'
#define tab '\t'
#define enter '\r'
#define delete '\177'

#define char_invisible 0
#define ctrl_l_char char_invisible
#define ctrl_r_char char_invisible
#define shift_l_char char_invisible
#define shift_r_char char_invisible
#define alt_l_char char_invisible
#define alt_r_char char_invisible
#define caps_lock_char char_invisible

#define shift_l_make 0x2a
#define shift_r_make 0x36 //竟然不是0xe0开头
#define ctrl_l_make 0x1d
#define ctrl_r_make 0xe01d
#define alt_l_make 0x38
#define alt_r_make 0xe038
#define caps_make 0x3a

void keyboard_init(void);
extern struct ioqueue kbd_buf;

#endif
