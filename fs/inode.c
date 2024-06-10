#include "inode.h"
#include "super_block.h"
#include "fs.h"
#include "string.h"
#include "list.h"
#include "thread.h"
#include "memory.h"
#include "stdio-kernel.h"
#include "interrupt.h"

struct inode* malloc_inode(void)
{
    struct task_struct* cur = running_thread();
    void* pdir_bak = cur->pdir;
    cur->pdir = NULL;
    struct inode* new_inode = sys_malloc(sizeof(struct inode));
    if (new_inode == NULL) {
        printk("malloc new inode failed");
        return NULL;
    }
    cur->pdir = pdir_bak;
    return new_inode;
}

void free_inode(struct inode* inode)
{
    struct task_struct* cur = running_thread();
    void* pdir_bak = cur->pdir;
    cur->pdir = NULL;
    sys_free(inode);
    cur->pdir = pdir_bak;
}

void inode_init(struct inode* inode,uint_32 ino)
{
    inode->ino = ino;
    inode->i_size = 0;
    inode->open_cnts = 0;
    inode->write_deny = false;

    for (uint_32 sec = 0 ; sec < 13 ; sec++) {
        inode->block[sec] = 0;
    }
}

struct inode_position get_ipos(struct partition* part,uint_32 ino)
{
    struct inode_position i_pos;
    struct super_block* sb = part->sb;
    uint_32 lba_off = ino*sizeof(struct inode)/BYTES_PER_SECTOR;
    i_pos.start_lba = sb->inode_array_lba + lba_off;
    i_pos.byte_offset = ino*sizeof(struct inode) - lba_off*BYTES_PER_SECTOR;
    i_pos.two_sec = (BYTES_PER_SECTOR < (i_pos.byte_offset+sizeof(struct inode)));
    return i_pos;
}

void sync_inode_array(struct partition* part,struct inode* inode,void* buf)
{
    struct inode_position i_pos = get_ipos(part,inode->ino);
    char* inode_buf = buf;
    uint_8 sec_op = i_pos.two_sec==true ? 2 : 1;

    struct inode pure_inode;  //我说怎么open_inode链表坏了，原来这里用了本体!!!
                              //赶紧改成复制体
    memcpy(&pure_inode,inode,sizeof(struct inode));
    pure_inode.open_cnts = 0;
    pure_inode.write_deny = false;
    pure_inode.inode_tag.next = pure_inode.inode_tag.prev = NULL; //后面还有一个(\笑哭)

    ide_read(part->hd,inode_buf,i_pos.start_lba,sec_op);
    memcpy(inode_buf+i_pos.byte_offset,&pure_inode,sizeof(struct inode));
    ide_write(part->hd,inode_buf,i_pos.start_lba,sec_op);
}

struct inode* inode_open(struct partition* part,uint_32 ino)
{
    struct list_elm* inode_elm = part->open_inodes.head.next;
    while (inode_elm != &part->open_inodes.tail)
    {
        struct inode* inode = mem2entry(struct inode,inode_elm,inode_tag);
        if (inode->ino == ino){
            inode->open_cnts++;
            return inode;
        }
        inode_elm = inode_elm->next;
    }

    struct inode* inode = malloc_inode();
    inode_init(inode,ino);
    struct inode_position i_pos = get_ipos(part,ino);
    uint_32 sec_op = i_pos.two_sec ? 2 : 1;
    char* inode_buf = sys_malloc(1024);
    if (inode_buf == NULL) {
        printk("inode buf malloc failed\n");
        return NULL;
    }
    ide_read(part->hd,inode_buf,i_pos.start_lba,sec_op);
    memcpy(inode,inode_buf+i_pos.byte_offset,sizeof(struct inode));
    inode->open_cnts = 1;
    list_push(&part->open_inodes,&inode->inode_tag);

    sys_free(inode_buf);
    return inode;
}

void inode_close(struct inode* inode)
{
    enum intr_status old_stat = intr_disable();
    if (--inode->open_cnts == 0)
    {
        list_remove(&inode->inode_tag);
        free_inode(inode);
    }
    intr_set_status(old_stat);
}

void inode_release(struct partition* part,uint_32 ino)
{
    struct inode* inode = inode_open(part,ino);
    uint_32* all_blocks = sys_malloc(sizeof(uint_32)*140);
    if (all_blocks == NULL) {
        printk("inode_release: sys_malloc failed\n");
        return ;
    }

    uint_32 btmp_idx;
    uint_32 blk_cnt = 12;
    for (uint_32 blk = 0 ; blk < 12 ; blk++) {
        all_blocks[blk] = inode->block[blk];
    }
    if (inode->block[12] != 0) {
        ide_read(part->hd,all_blocks+12,inode->block[12],1);
        btmp_idx = inode->block[12] - part->sb->data_start_lba;
        bitmap_set(&part->block_bitmap,btmp_idx,0);
        sync_bitmap(part,btmp_idx,BLOCK_BITMAP);
        blk_cnt = 140;
    }

    for (uint_32 blk = 0 ; blk < blk_cnt ; blk++)
    {
        if (all_blocks[blk] == 0) continue;
        btmp_idx = all_blocks[blk] - part->sb->data_start_lba;
        bitmap_set(&part->block_bitmap,btmp_idx,0);
        sync_bitmap(part,btmp_idx,BLOCK_BITMAP);
    }
    void* io_buf = sys_malloc(BLOCKSIZE*2);
    if (io_buf == NULL) {
        printk("iobuf malloc failed\n");
        return ;
    }
    bitmap_set(&part->inode_bitmap,inode->ino,0);
    sync_bitmap(part,inode->ino,INODE_BITMAP);
    inode_close(inode);
    sys_free(io_buf);
    sys_free(all_blocks);
}




