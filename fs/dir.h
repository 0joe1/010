#ifndef FS_DIR_H
#define FS_DIR_H
#include "stdint.h"
#include "inode.h"

#define FILENAME_MAXLEN 20
enum filetype {
    FT_UNKNOWN,  //默认为0,unknown
    FT_REGULAR,
    FT_DIRECTORY,
};

struct dir {
    struct inode* inode;
    uint_32 dir_pos;
    char dir_buf[512];
};

struct dir_entry {
    uint_32 ino;
    char filename[FILENAME_MAXLEN];
    enum filetype ftype;
};

void open_root_dir(struct partition* part);
struct dir* open_dir(struct partition* part,uint_32 ino);
void close_dir(struct dir* dir);
void dir_entry_init(struct dir_entry* de,uint_32 ino,const char* fname,enum filetype type);
Bool sync_dir_entry(struct dir* p_dir,struct dir_entry* de,void* io_buf);
Bool search_dir_entry(struct partition* part,struct dir* pdir,const char* fname,struct dir_entry* de);
Bool delete_dir_entry(struct partition* part,struct dir* pdir,uint_32 ino,void* io_buf);
struct dir_entry* dir_read(struct dir* dir);
int_32 dir_remove(struct dir* pdir,struct dir* cdir);
Bool dir_empty(struct partition* part,struct dir* dir);
uint_32 get_parent_inode_nr(struct partition* part,uint_32 cino,void* io_buf);
Bool get_child_dirname(struct partition* part,uint_32 pino,uint_32 cino,char* path,void* io_buf);

#endif
