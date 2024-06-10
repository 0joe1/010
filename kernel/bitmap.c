#include "bitmap.h"
#include "memory.h"
#include "debug.h"
#include "string.h"

void bit_init(struct bitmap* btmp)
{
    memset(btmp->bits,0,btmp->map_size);
}

int bit_scan_test(struct bitmap* btmp,uint_32 bit_idx)
{
    uint_32 byte_idx = bit_idx / 8;
    uint_32 bit_odd  = bit_idx % 8;
    return btmp->bits[byte_idx] & (BIT_MASK << bit_odd);
}

int bit_scan(struct bitmap* btmp,uint_32 bit_cnt)
{
    uint_32 bit_byte = 0;
    while (bit_byte < btmp->map_size && (0xff == btmp->bits[bit_byte]) ){
        bit_byte++;
    }
    ASSERT(bit_byte < btmp->map_size);
    if (bit_byte == btmp->map_size){
        return -1;
    }

    uint_32 bit_idx = bit_byte*8;
    uint_32 count = 0;
    int ret = -1;    //失败时需要返回-1
    while (bit_idx < btmp->map_size * 8)
    {
        if (bit_scan_test(btmp,bit_idx) == 0){
            ++count;
        } else {
            count = 0;
        }
        if (count == bit_cnt){
            ret = bit_idx-count+1;
            break;
        }
        ++bit_idx;
    }

    return ret;
}
void bitmap_set(struct bitmap* btmp,uint_32 bit_idx,uint_8 value)
{
    ASSERT(value==0 || value==1);
    uint_32 bit_byte = bit_idx / 8;
    uint_32 bit_odd  = bit_idx % 8;
    if (value) {
        btmp->bits[bit_byte] |= (BIT_MASK << bit_odd);
    }else {
        btmp->bits[bit_byte] &= ~(BIT_MASK << bit_odd);
    }
}
