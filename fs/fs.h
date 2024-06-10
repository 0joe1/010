#ifndef FS_H
#define FS_H
#include "ide.h"
#include "super_block.h"
#include "inode.h"
#include "dir.h"

#define LINFSMAGIC 0x27308192
#define MFILES_PER_PARTITION 4096
#define BYTES_PER_SECTOR 512
#define BITS_PER_SECTOR (BYTES_PER_SECTOR*8)
#define BLOCKSIZE BYTES_PER_SECTOR

#define MAX_PATH_LEN 128

extern struct channel channel[];
extern struct list partition_list;

extern struct partition* cur_part;
extern struct dir root;

enum btmp_type {
    INODE_BITMAP,
    BLOCK_BITMAP
};

enum standard {
    stdin,
    stdout,
    stderr
};

enum whence {
    SEEK_SET = 1,
    SEEK_CUR,
    SEEK_END
};

struct path_search_record {
    char searched_path[MAX_PATH_LEN];
    enum filetype ftype;
    struct dir* p_dir;
};

struct stat {
    uint_32 st_ino;
    uint_32 st_fsize;
    enum filetype st_ftype;
};

void fs_format(struct partition*);
void fs_init(void);
Bool mount_partition(struct list_elm* pt_elm,int arg);
int_32 inode_bitmap_alloc(struct partition* part);
int_32 block_bitmap_alloc(struct partition* part);
void sync_bitmap(struct partition* part,uint_32 bit_idx,uint_8 type);
int_32 sys_open(const char* filename,uint_8 flag);
const char* path_parse(const char* path,char* name);
uint_32 path_depth_cnt(const char* path);
int_32 search_file(const char* filename,struct path_search_record* record);
int_32 sys_close(int_32 fd);
int_32 sys_write(uint_32 fd,const void* buf,uint_32 count);
int_32 sys_read(uint_32 fd,void* buf,uint_32 count);
int_32 sys_lseek(uint_32 fd,int_32 offset,uint_8 whence);
int_32 sys_unlink(const char* filename);
int_32 sys_mkdir(const char* pathname);
struct dir* sys_opendir(const char* pathname);
int_32 sys_closedir(struct dir* dir);
struct dir_entry* sys_readdir(struct dir* dir);
void sys_rewinddir(struct dir* dir);
int_32 sys_rmdir(const char* pathname);
int_32 sys_getcwd(char* buf,uint_32 size);
int_32 sys_chdir(const char* path);
int_32 sys_stat(const char* path,struct stat* fstat);
void sys_help(void);

#endif
