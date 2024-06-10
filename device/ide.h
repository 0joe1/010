#ifndef DEVICE_IDE_H
#define DEVICE_IDE_H
#include "stdint.h"
#include "sync.h"
#include "memory.h"

#define max_lba ((80*1024*1024/512) - 1)

struct partition {
    char name[17];
    struct disk* hd;
    struct super_block* sb;
    uint_32 start_sec;
    uint_32 sec_cnt;
    struct bitmap block_bitmap;
    struct bitmap inode_bitmap;
    struct list open_inodes;
    struct list_elm part_elm;
};
struct disk {
    char name[17];
    struct channel* chnl;
    uint_32 dev_no;
    struct partition primary[4];
    struct partition logic[7];
};

struct channel {
    uint_32 port_base;
    uint_32 int_no;
    Bool expecting_intr;
    struct lock lock;
    struct semaphore wait;  //不能放在disk里，若放disk，intr_hd_handler不知道是哪个disk
    struct disk disk[2];
};

struct partition_table_entry {
    uint_8 boot_sign;
    uint_8 start_head;
    uint_8 start_sector;    //低6位
    uint_8 start_cylinder;  //第三字节高2位+第四字节
    uint_8 par_type;
    uint_8 end_head;
    uint_8 end_sector;      //低6位
    uint_8 end_cylinder;    //第七字节高2位+第八字节
    uint_32 start_lba;
    uint_32 sec_cnt;
} __attribute__ ((packed));

struct boot_record {
    char boot[446];
    struct partition_table_entry part_entry[4];
    uint_16 signature;
} __attribute__ ((packed));


static Bool busy_wait(struct disk* hd); 
void ide_init(void);
void select_disk(struct disk* hd);
void select_sector(struct disk* hd,uint_32 start_lba,uint_8 sec_cnt);
void cmd_out(struct disk* hd,uint_8 cmd);
void ide_read(struct disk* hd,void* buf,uint_32 start_lba,uint_32 sec_cnt);
void ide_write(struct disk* hd,void* buf,uint_32 lba,uint_32 sec_cnt);
void identify_disk(struct disk* hd);
void swap_pair_bytes(const char* src,char* dst,uint_32 len);
void intr_hd_handler(uint_32 int_num);
void partition_scan(struct disk* hd,uint_32 base_lba);
Bool partition_info(struct list_elm* ptelm,int arg);

#endif
