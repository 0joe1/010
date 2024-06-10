#include "dir.h"
#include "string.h"
#include "memory.h"
#include "stdio-kernel.h"
#include "fs.h"
#include "debug.h"
#include "inode.h"

void open_root_dir(struct partition* part)
{
    ASSERT(part != NULL);
    root.inode = inode_open(part,part->sb->root_inode_no);
    root.dir_pos = 0;
    memset(root.dir_buf,0,512);
}

struct dir* open_dir(struct partition* part,uint_32 ino)
{
    if (ino == 0) {
        root.dir_pos = 0;
        memset(root.dir_buf,0,sizeof(root.dir_buf));
        return &root;
    }
    struct dir* new_dir = sys_malloc(sizeof(struct dir));
    new_dir->inode = inode_open(part,ino);
    new_dir->dir_pos = 0;
    memset(new_dir->dir_buf,0,512);
    return new_dir;
}

void close_dir(struct dir* dir)
{
    if (dir == &root) {
        return ;
    }
    inode_close(dir->inode);
    sys_free(dir);
}

void dir_entry_init(struct dir_entry* de,uint_32 ino,const char* fname,enum filetype type)
{
    de->ino = ino;
    strcpy(de->filename,fname);
    de->ftype = type;
}

Bool sync_dir_entry(struct dir* p_dir,struct dir_entry* de,void* io_buf)
{
    struct super_block* sb = cur_part->sb;
    uint_32* all_blocks = sys_malloc(140*sizeof(uint_32));  //128+12 = 140
    if (all_blocks == NULL) {
        printk("malloc all_blocks failed\n");
        return false;
    }

    struct inode* d_inode = p_dir->inode;
    for (uint_32 blk = 0 ; blk < 12 ; blk++) {
        all_blocks[blk] = d_inode->block[blk];
    }
    if (d_inode->block[12] != 0) {
        ide_read(cur_part->hd,all_blocks+12,d_inode->block[12],1);
    }

    for (uint_32 blk = 0 ; blk < 140 ; blk++)
    {
        if (all_blocks[blk] == 0)
        {
            uint_32 new_sec = block_bitmap_alloc(cur_part);
            if (new_sec == -1) {
                printk("new sec alloc failed\n");
                sys_free(all_blocks);
                return false;
            }
            d_inode->block[blk] = new_sec;

            if (blk == 12)
            {
                uint_32 another_new = block_bitmap_alloc(cur_part);
                if (another_new == -1) {
                    printk("another_new alloc failed\n");
                    d_inode->block[blk] = 0;
                    bitmap_set(&cur_part->block_bitmap,new_sec - sb->data_start_lba,0);
                    sys_free(all_blocks);
                    return false;
                }
                sync_bitmap(cur_part,new_sec - sb->data_start_lba,BLOCK_BITMAP);
                d_inode->block[blk] = another_new;
                sync_bitmap(cur_part,another_new - sb->data_start_lba,BLOCK_BITMAP);
                all_blocks[12] = another_new;
            }
            else
            {
                all_blocks[blk] = new_sec;
            }

            memset(io_buf,0,1024);
            struct dir_entry* de_buf = io_buf;
            memcpy(de_buf,de,sizeof(struct dir_entry));
            ide_write(cur_part->hd,de_buf,all_blocks[blk],1);
            p_dir->inode->i_size += sizeof(struct dir_entry);
            return true;
        }

        ide_read(cur_part->hd,io_buf,all_blocks[blk],1);
        struct dir_entry* de_ptr = io_buf;
        uint_32 max_de_cnt = BYTES_PER_SECTOR/sizeof(struct dir_entry);
        for (uint_32 de_idx = 0 ; de_idx < max_de_cnt ; de_idx++,de_ptr++)
        {
            if (de_ptr->ftype != FT_UNKNOWN) continue;
            memcpy(de_ptr,de,sizeof(struct dir_entry));
            ide_write(cur_part->hd,io_buf,all_blocks[blk],1);
            p_dir->inode->i_size += sizeof(struct dir_entry);
            memset(io_buf,0,1024);
            sync_inode_array(cur_part,p_dir->inode,io_buf);
            return true;
        }
    }

    sys_free(all_blocks);
    return false;
}

Bool search_dir_entry(struct partition* part,struct dir* pdir,const char* fname,struct dir_entry* de)
{
    uint_32 blk_cnt = 140;
    uint_32* all_blocks = sys_malloc(140*sizeof(uint_32));
    if (all_blocks == NULL) {
        printk("search_file: all_blocks malloc failed\n");
        sys_free(all_blocks);
        return false;
    }

    struct inode* p_inode = pdir->inode;
    for (uint_32 blk = 0 ; blk < 12 ; blk++) {
        all_blocks[blk] = p_inode->block[blk];
    }
    if (all_blocks[12] != 0) {
        ide_read(cur_part->hd,all_blocks+12,p_inode->block[12],1);
    } else {
        blk_cnt = 12;
    }

    uint_32 max_dir_cnt = BYTES_PER_SECTOR/sizeof(struct dir_entry);
    struct dir_entry* de_buf = sys_malloc(BYTES_PER_SECTOR);
    struct dir_entry* dep = de_buf;
    for (uint_32 blk = 0 ; blk < blk_cnt ; blk++)
    {
        ide_read(part->hd,de_buf,all_blocks[blk],1);
        for (uint_32 d = 0 ; d < max_dir_cnt ; d++,dep++) {
            if (!strcmp(fname,dep->filename)) {
                memcpy(de,dep,sizeof(struct dir_entry));
                sys_free(all_blocks);
                sys_free(de_buf);
                return true;
            }
        }
        dep = de_buf;
        memset(de_buf,0,512);
    }
    sys_free(all_blocks);
    sys_free(de_buf);
    return false;
}

Bool delete_dir_entry(struct partition* part,struct dir* pdir,uint_32 ino,void* io_buf)
{
    if (ino == 0) {
        return false;
    }
    uint_32* all_blocks = sys_malloc(sizeof(uint_32)*140);
    if (all_blocks == NULL) {
        printk("delete_dir_entry: all_blocks malloc failed\n");
        return -1;
    }

    struct inode* p_inode = pdir->inode;
    for (uint_32 blk = 0 ; blk < 12 ; blk++) {
        all_blocks[blk] = p_inode->block[blk];
    }
    if (p_inode->block[12] != 0) {
        ide_read(part->hd,all_blocks+12,p_inode->block[12],1);
    }

    struct super_block* sb = part->sb;
    uint_32 max_dir_cnt = BLOCKSIZE/sizeof(struct dir_entry);
    for (uint_32 blk = 0 ; blk < 140 ; blk++)
    {
        if (all_blocks[blk] == 0) continue;

        memset(io_buf,0,BLOCKSIZE);
        ide_read(part->hd,io_buf,all_blocks[blk],1);
        struct dir_entry* de_buf = io_buf;
        struct dir_entry* de_found = NULL;
        uint_32 dir_cnt = 0;
        for (uint_32 di = 0 ; di < max_dir_cnt ; di++) {
            if ((de_buf+di)->ftype == FT_UNKNOWN) continue;
            dir_cnt++;
            if ((de_buf+di)->ino == ino) {
                de_found = de_buf + di;
                continue;
            }
        }
        if (de_found == NULL){
            continue;
        }

        ASSERT(dir_cnt > 0);
        uint_32 block_btmp_idx;
        if (dir_cnt > 1)
        {
            memset(de_found,0,sizeof(struct dir_entry));
            ide_write(part->hd,io_buf,all_blocks[blk],1);
        }
        else
        {
            block_btmp_idx = all_blocks[blk] - sb->data_start_lba;
            bitmap_set(&part->block_bitmap,block_btmp_idx,0);
            sync_bitmap(part,block_btmp_idx,BLOCK_BITMAP);
            if (blk < 12)
            {
                p_inode->block[blk] = 0;
            }
            else
            {
                uint_32 indirect_blkcnt = 0;
                for (uint_32 b = 12 ; b < 140 ; b++) {
                    if (all_blocks[b] != 0) indirect_blkcnt++;
                }
                ASSERT(indirect_blkcnt != 0);

                if (indirect_blkcnt > 1) 
                {
                    all_blocks[blk] = 0;
                    ide_write(part->hd,all_blocks+12,all_blocks[12],1);
                }
                else
                {
                    block_btmp_idx = p_inode->block[12] - sb->data_start_lba;
                    bitmap_set(&part->block_bitmap,block_btmp_idx,0);
                    sync_bitmap(part,block_btmp_idx,BLOCK_BITMAP);
                    p_inode->block[12] = 0;
                }
            }
        }
        memset(io_buf,0,BLOCKSIZE*2);
        pdir->inode->i_size -= sizeof(struct dir_entry);
        sync_inode_array(part,p_inode,io_buf);
        return true;
    }
    sys_free(all_blocks);
    return false;
}

struct dir_entry* dir_read(struct dir* dir)
{
    if (dir->dir_pos >= dir->inode->i_size) {
        return NULL;
    }
    struct dir_entry* de = (struct dir_entry*)dir->dir_buf;
    uint_32* all_blocks = sys_malloc(sizeof(uint_32)*140);
    if (all_blocks == NULL) {
        printk("dir_read: all_blocks alloc failed\n");
        return NULL;
    }
    for (uint_32 blk = 0 ; blk < 12 ; blk++) {
        all_blocks[blk] = dir->inode->block[blk];
    }
    if(all_blocks[12] != 0) {
        ide_read(cur_part->hd,all_blocks+12,dir->inode->block[12],1);
    }

    uint_32 cur_blkidx = 0;
    uint_32 cur_entry_pos = 0;
    uint_32 max_de_cnt = BLOCKSIZE / cur_part->sb->dir_entry_size;
    while(cur_blkidx < 140)
    {
        if (all_blocks[cur_blkidx] == 0){
            continue;
        }
        ide_read(cur_part->hd,dir->dir_buf,all_blocks[cur_blkidx],1);

        for (uint_32 d_off = 0 ; d_off < max_de_cnt ; d_off++)
        {
            if ((de+d_off)->ftype == FT_UNKNOWN) continue;
            if (cur_entry_pos != dir->dir_pos) {
                cur_entry_pos += cur_part->sb->dir_entry_size;
                continue;
            }

            dir->dir_pos += cur_part->sb->dir_entry_size;
            return (de+d_off);
        }
        cur_blkidx++;
    }

    sys_free(all_blocks);
    return NULL;
}

int_32 dir_remove(struct dir* pdir,struct dir* cdir)
{
    struct inode* inode = cdir->inode;
    for (uint_32 blkidx = 1 ; blkidx < 13 ; blkidx++) {
        ASSERT(inode->block[blkidx] == 0);
    }

    void* io_buf = sys_malloc(BLOCKSIZE*2);
    if (io_buf == NULL) {
        printk("dir_remove: io_buf alloc failed\n");
        return -1;
    }

    delete_dir_entry(cur_part,pdir,inode->ino,io_buf);
    inode_release(cur_part,inode->ino);

    close_dir(cdir);
    sys_free(io_buf);
    return 0;
}

Bool dir_empty(struct partition* part,struct dir* dir) {
    return (dir->inode->i_size == 2*part->sb->dir_entry_size);
}

uint_32 get_parent_inode_nr(struct partition* part,uint_32 cino,void* io_buf)
{
    struct inode* cinode = inode_open(part,cino);
    struct dir_entry* de = io_buf;
    ide_read(part->hd,io_buf,cinode->block[0],1);
    inode_close(cinode);
    return de[1].ino;
}

Bool get_child_dirname(struct partition* part,uint_32 pino,uint_32 cino,char* path,void* io_buf)
{
    struct inode* p_inode = inode_open(part,pino);
    if (p_inode == NULL) {
        printk("get_child_dirname: p_inode open failed\n");
        return 0;
    }

    uint_32 blkcnt = 12;
    uint_32* all_blocks = sys_malloc(sizeof(uint_32)*140);
    if (all_blocks == NULL) {
        printk("get_child_dirname: sys_malloc failed\n");
        return 0;
    }
    for (uint_32 blk = 0 ; blk < 12 ; blk++) {
        all_blocks[blk] = p_inode->block[blk];
    }
    if (p_inode->block[12] != 0) {
        ide_read(part->hd,all_blocks+12,p_inode->block[12],1);
        blkcnt = 140;
    }

    uint_32 max_dir_cnt = BLOCKSIZE / part->sb->dir_entry_size;
    struct dir_entry* de = io_buf;
    for (uint_32 blk = 0 ; blk < blkcnt ; blk++)
    {
        if (all_blocks[blk] == 0) continue;
        ide_read(cur_part->hd,io_buf,all_blocks[blk],1);
        for (uint_32 d_off = 0 ; d_off < max_dir_cnt ; d_off++)
        {
            if ((de+d_off)->ftype == FT_UNKNOWN) continue;
            if ((de+d_off)->ino == cino) {
                strcat(path,"/");
                strcat(path,(de+d_off)->filename);
                sys_free(all_blocks);
                return 1;
            }
        }
    }

    sys_free(all_blocks);
    return false;
}


