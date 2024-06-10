#include "file.h"
#include "debug.h"
#include "memory.h"
#include "string.h"
#include "stdio-kernel.h"
#include "fs.h"
#include "inode.h"
#include "thread.h"
#include "interrupt.h"
#include "global.h"

struct file file_table[MAX_OPEN_FILES];

int_32 get_free_slot_filetable(void)
{
    for (uint_32 idx = 3 ; idx < MAX_OPEN_FILES ; idx++) {
        if (file_table[idx].inode == NULL)  return idx;
    }
    return -1;
}

int_32 pcb_fd_install(uint_32 fd_idx)
{
    struct task_struct* cur = running_thread();
    for (uint_32 fd = 3 ; fd < MAX_OPEN_FILES_PROC ; fd++)
    {
        if (cur->fd_table[fd] != -1) continue;
        cur->fd_table[fd] = fd_idx;
        return fd;
    }
    return -1;
}

int_32 file_create(struct dir* parent_dir,const char* filename,uint_8 flag)
{
    void* io_buf = sys_malloc(1024);
    if (io_buf == NULL) {
        printk("file create failed: malloc iobuf");
        return -1;
    }

    uint_32 rollback = 0;
    uint_32 ino_idx = inode_bitmap_alloc(cur_part);
    if (ino_idx == -1) {
        printk("inode bitamp alloc failed");
        return -1;
    }

    /* make new inode */
    struct inode* new_inode = malloc_inode();
    if (new_inode == NULL) {
        rollback = 1;
        printk("malloc inode failed\n");
        goto roll_back;
    }
    inode_init(new_inode,ino_idx);

    /* file_table */
    int_32 fidx = get_free_slot_filetable();
    if (fidx == -1) {
        rollback = 2;
        printk("get free slot filetable failed");
        goto roll_back;
    }
    file_table[fidx].inode  = new_inode;
    file_table[fidx].fd_pos = 0;
    file_table[fidx].flag   = flag;

    /* revise parent dir */
    struct dir_entry de;
    memset(&de,0,sizeof(de));
    dir_entry_init(&de,ino_idx,filename,FT_REGULAR);
    if (!sync_dir_entry(parent_dir,&de,io_buf)) {
        rollback = 3;
        printk("sync_dir_entry failed\n");
        goto roll_back;
    }

    /* sync to disk */
    sync_bitmap(cur_part,ino_idx,INODE_BITMAP);
    memset(io_buf,0,1024);
    sync_inode_array(cur_part,parent_dir->inode,io_buf);
    memset(io_buf,0,1024);
    sync_inode_array(cur_part,new_inode,io_buf);

    new_inode->open_cnts = 1;
    list_push(&cur_part->open_inodes,&new_inode->inode_tag);

    sys_free(io_buf);
    return pcb_fd_install(fidx);

roll_back:
    switch(rollback){
        case 3:
            memset(&file_table[fidx],0,sizeof(struct file));
        case 2:
            free_inode(new_inode);
        case 1:
            bitmap_set(&cur_part->inode_bitmap,ino_idx,0);
            sys_free(io_buf);
    }
    return -1;
}

int_32 file_open(uint_32 ino,uint_8 flag)
{
    struct inode* open_inode = inode_open(cur_part,ino);
    Bool* deny_ptr;
    uint_32 fidx = get_free_slot_filetable();
    if (fidx == -1){
        printk("file open: get_free_slot_filetable failed\n");
        return -1;
    }
    file_table[fidx].inode  = open_inode;
    file_table[fidx].fd_pos = 0;
    file_table[fidx].flag   = flag;

    enum intr_status old_status = intr_disable();
    deny_ptr = &open_inode->write_deny;
    if (*deny_ptr == true){
        intr_set_status(old_status);
        printk("can't open when another thread is writing\n");
        return -1;
    }
    *deny_ptr = false;
    intr_set_status(old_status);

    return pcb_fd_install(fidx);
}

int_32 file_close(struct file* file)
{
    if (file == NULL) {
        return -1;
    }
    file->inode->write_deny = false;
    inode_close(file->inode);
    file->inode = NULL;
    return 0;
}

int_32 blk_alloc_sync(struct partition* part,struct inode* inode,uint_32* all_blocks,uint_32 blkidx)
{
    uint_32 lba = block_bitmap_alloc(part);
    uint_32 bit_idx = lba - part->sb->data_start_lba;
    inode->block[blkidx] = all_blocks[blkidx] = lba;
    sync_bitmap(part,bit_idx,BLOCK_BITMAP);
    return lba;
}

int_32 file_write(struct file* file,const void* buf,uint_32 count)
{
    if ((file->inode->i_size + count) >= MAX_FILESIZE){
        printk("exceed max filesize\n");
        return -1;
    }
    const char* src = buf;
    uint_32* all_blocks = sys_malloc(sizeof(uint_32)*140);
    if (all_blocks == NULL) {
        printk("file_write: all_blocks alloc failed\n");
        return -1;
    }
    char* io_buf = sys_malloc(1024);
    if (io_buf == NULL) {
        sys_free(io_buf);
        printk("file_write: io_buf alloc failed\n");
        return -1;
    }

    //printk("inode table start at:0x%x\n",cur_part->sb->inode_array_lba);
    //printk("data start at:0x%x\n",cur_part->sb->data_start_lba);
    struct inode* inode = file->inode;
    ASSERT(inode != NULL);

    uint_32 cur_blksize = DIV_ROUND_UP(inode->i_size,BLOCKSIZE);
    uint_32 fu_blksize = DIV_ROUND_UP(inode->i_size+count,BLOCKSIZE);
    uint_32 add_blks = fu_blksize - cur_blksize;


    uint_32 cur_blkidx = inode->i_size / BLOCKSIZE;
    int_32 lba;
    if (cur_blksize == 0) {
        lba = block_bitmap_alloc(cur_part);
        inode->block[0] = lba;
        uint_32 bit_idx = lba - cur_part->sb->data_start_lba;
        sync_bitmap(cur_part,bit_idx,BLOCK_BITMAP);
        cur_blkidx = 0;
        add_blks--;
    }
    all_blocks[cur_blkidx++] = inode->block[0];
    if (fu_blksize <= 12)
    {
        while (add_blks--) {
            if ((lba = blk_alloc_sync(cur_part,inode,all_blocks,cur_blkidx)) == -1) {
                sys_free(io_buf);
                sys_free(all_blocks);
                printk("file_write: blk_alloc_sync failed\n");
                return -1;
            }
            cur_blkidx++;
        }
    }
    else if (cur_blksize <= 12 && fu_blksize > 12)
    {
        if ((lba = blk_alloc_sync(cur_part,inode,all_blocks,12)) == -1) {
            sys_free(io_buf);
            sys_free(all_blocks);
            printk("file_write: blk_alloc_sync failed\n");
            return -1;
        }

        for (; cur_blkidx < 12 ; cur_blkidx++,add_blks--) {
            if ((lba = blk_alloc_sync(cur_part,inode,all_blocks,cur_blkidx)) == -1) {
                sys_free(io_buf);
                sys_free(all_blocks);
                printk("file_write: blk_alloc_sync failed\n");
                return -1;
            }
        }
        while (add_blks--){
            if ((lba = block_bitmap_alloc(cur_part)) == -1) {
                sys_free(io_buf);
                sys_free(all_blocks);
                printk("file_write: block bitmap alloc failed\n");
                return -1;
            }
            uint_32 bit_idx = lba - cur_part->sb->data_start_lba;
            sync_bitmap(cur_part,bit_idx,BLOCK_BITMAP);
            all_blocks[cur_blkidx++] = lba;
        }
        ide_write(cur_part->hd,all_blocks+12,inode->block[12],1);
    }
    else
    {
        while (add_blks--){
            if ((lba = block_bitmap_alloc(cur_part)) == -1) {
                sys_free(io_buf);
                sys_free(all_blocks);
                printk("file_write: block bitmap alloc failed\n");
                return -1;
            }
            uint_32 bit_idx = lba - cur_part->sb->data_start_lba;
            sync_bitmap(cur_part,bit_idx,BLOCK_BITMAP);
            all_blocks[cur_blkidx++] = lba;
        }
        ide_write(cur_part->hd,all_blocks+12,inode->block[12],1);
    }

    uint_32 blk_idx = cur_blksize>0 ? cur_blksize - 1 : 0;
    uint_32 size_left = count;
    uint_32 sec_start_byte;
    uint_32 sec_remain_bytes;
    uint_32 size_tow;
    while (size_left)
    {
        ASSERT(blk_idx < fu_blksize);
        sec_start_byte = inode->i_size % BLOCKSIZE;
        sec_remain_bytes = BLOCKSIZE - sec_start_byte;
        size_tow  = size_left < sec_remain_bytes ? size_left : sec_remain_bytes;
        memset(io_buf,0,512);
        if (blk_idx == cur_blksize-1) {
            ide_read(cur_part->hd,io_buf,all_blocks[blk_idx],1);
        }
        memcpy(io_buf+sec_start_byte,(void*)src,size_tow);

        ide_write(cur_part->hd,io_buf,all_blocks[blk_idx++],1);
        //printk("file write at lba 0x%x\n",all_blocks[blk_idx-1]);
        src += size_tow;
        inode->i_size += size_tow;
        size_left -= size_tow;
    }
    memset(io_buf,0,1024);
    sync_inode_array(cur_part,inode,io_buf);

    sys_free(io_buf);
    sys_free(all_blocks);
    return count-size_left;
}

int_32 file_read(struct file* file,void* buf,uint_32 count)
{
    if ((file->fd_pos + count) >= file->inode->i_size){
        count = file->inode->i_size - file->fd_pos;
        if (count == 0) {
            //printk("read the end of file\n");
            return -1;
        }
    }
    if (count == 0){ return 0; }

    char* dst = buf;
    uint_32* all_blocks = sys_malloc(sizeof(uint_32)*140);
    if (all_blocks == NULL) {
        printk("file_write: all_blocks alloc failed\n");
        return -1;
    }
    char* io_buf = sys_malloc(BLOCKSIZE);
    if (io_buf == NULL) {
        sys_free(io_buf);
        printk("file_write: io_buf alloc failed\n");
        return -1;
    }

    struct inode* inode = file->inode;
    ASSERT(inode != NULL);

    uint_32 start_idx = file->fd_pos / BLOCKSIZE;
    uint_32 end_idx   = DIV_ROUND_UP(file->fd_pos+count,BLOCKSIZE) - 1;
    uint_32 blks_to_read = end_idx - start_idx + 1;

    uint_32 cur_blkidx = start_idx;
    if (end_idx < 12)
    {
        while (blks_to_read--) {
            all_blocks[cur_blkidx] = inode->block[cur_blkidx];
            cur_blkidx++;
        }
    }
    else if (start_idx < 12 && end_idx >= 12)
    {
        for (; cur_blkidx < 12 ; cur_blkidx++) {
            all_blocks[cur_blkidx] = inode->block[cur_blkidx];
        }
        ide_read(cur_part->hd,all_blocks+12,inode->block[12],1);
    }
    else
    {
        ide_read(cur_part->hd,all_blocks+12,inode->block[12],1);
    }

    cur_blkidx = start_idx;
    uint_32 size_left = count;
    uint_32 sec_start_byte;
    uint_32 sec_remain_bytes;
    uint_32 size_rd;
    while (size_left)
    {
        ASSERT(cur_blkidx <= end_idx);
        sec_start_byte = file->fd_pos % BLOCKSIZE;
        sec_remain_bytes = BLOCKSIZE - sec_start_byte;
        size_rd  = size_left < sec_remain_bytes ? size_left : sec_remain_bytes;
        memset(io_buf,0,512);
        ide_read(cur_part->hd,io_buf,all_blocks[cur_blkidx++],1);
        memcpy(dst,io_buf+sec_start_byte,size_rd);

        file->fd_pos += size_rd;
        dst += size_rd;
        size_left -= size_rd;
    }

    sys_free(io_buf);
    sys_free(all_blocks);
    return count-size_left;
}

