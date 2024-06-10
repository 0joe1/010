#ifndef FS_FILE_H
#define FS_FILE_H
#include "inode.h"
#include "dir.h"
#include "ide.h"

#define MAX_OPEN_FILES 17
#define MAX_FILESIZE 512*140

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWT 2
#define O_CREAT 4

struct file {
    struct inode* inode;
    uint_32 fd_pos;
    uint_32 flag;
};
extern struct file file_table[];

int_32 file_create(struct dir* parent_dir,const char* filename,uint_8 flag);
int_32 file_open(uint_32 ino,uint_8 flag);
int_32 file_close(struct file* file);
int_32 get_free_slot_filetable(void);
int_32 pcb_fd_install(uint_32 fd_idx);
int_32 blk_alloc_sync(struct partition* part,struct inode* inode,uint_32* all_blocks,uint_32 blkidx);
int_32 file_write(struct file* file,const void* buf,uint_32 count);
int_32 file_read(struct file* file,void* buf,uint_32 count);

#endif
