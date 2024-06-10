#ifndef FS_INODE_H
#define FS_INODE_H
#include "stdint.h"
#include "list.h"
#include "ide.h"

struct inode {
    uint_32 ino;
    uint_32 i_size;
    uint_32 block[13];
    uint_32 open_cnts;
    Bool write_deny;
    struct list_elm inode_tag;
};

struct inode_position {
    Bool two_sec;
    uint_32 start_lba;
    uint_32 byte_offset;
};

void inode_init(struct inode* inode,uint_32 ino);
struct inode_position get_ipos(struct partition* part,uint_32 ino);
void sync_inode_array(struct partition* part,struct inode* inode,void* buf);
struct inode* malloc_inode(void);
void free_inode(struct inode* inode);
struct inode* inode_open(struct partition* part,uint_32 ino);
void inode_close(struct inode* inode);
void inode_release(struct partition* part,uint_32 ino);

#endif
