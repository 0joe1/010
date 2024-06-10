#ifndef KERNEL_BITMAP_H
#define KERNEL_BITMAP_H
#include "stdint.h"

#define BIT_MASK 1

struct bitmap {
    uint_32 map_size;
    uint_8* bits;
};

void bit_init(struct bitmap* btmp);
int bit_scan_test(struct bitmap* btmp,uint_32 bit_idx);
int bit_scan(struct bitmap* btmp,uint_32 cnt);
void bitmap_set(struct bitmap* btmp,uint_32 bit_idx,uint_8 value);

#endif
