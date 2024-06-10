#include "stdint.h"
#ifndef IO_H
#define IO_H

static inline void outb(uint_16 port,uint_8 data){
    asm volatile ("outb %%al,%%dx"::"a"(data),"d"(port));
}

static inline void outsw(uint_16 port,void* buf,uint_32 wcnt){
    asm volatile ("cld; rep outsw;":"+S"(buf),"+c"(wcnt):"d"(port));
}

static inline uint_8 inb(uint_16 port) {
    uint_8 data;
    asm volatile ("inb %%dx,%b0":"=a"(data):"d"(port));
    return data;
}

static inline void insw(uint_16 port,void* buf,uint_32 wcnt) {
    asm volatile("cld; rep insw;":"+D"(buf),"+c"(wcnt):"d"(port):"memory");
}

#endif
