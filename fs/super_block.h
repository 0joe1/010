#ifndef FS_SUPER_BLOCK_H
#define FS_SUPER_BLOCK_H
#include "stdint.h"

struct super_block {
    uint_32 magic;
    uint_32 block_cnt;
    uint_32 inode_cnt;
    uint_32 part_lba;
    uint_32 block_bitmap_lba;
    uint_32 block_bitmap_sects;
    uint_32 inode_bitmap_lba;
    uint_32 inode_bitmap_sects;
    uint_32 inode_array_lba;
    uint_32 inode_array_sects;

    uint_32 data_start_lba;
    uint_32 root_inode_no;
    uint_32 dir_entry_size;
    /* 13*4 = 52 */
    char pad[460]; //460+52=512
} __attribute__ ((packed));

#endif
