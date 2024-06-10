#include "fs.h"
#include "pipe.h"
#include "thread.h"
#include "global.h"
#include "stdio-kernel.h"
#include "memory.h"
#include "string.h"
#include "dir.h"
#include "debug.h"
#include "list.h"
#include "file.h"
#include "console.h"
#include "keyboard.h"
#include "ioqueue.h"

struct partition* cur_part;
struct dir root;

void fs_format(struct partition* p)
{
    struct super_block sb;
    sb.magic = LINFSMAGIC;
    sb.dir_entry_size = sizeof(struct dir_entry);
    sb.block_cnt = p->sec_cnt;
    sb.inode_cnt = MFILES_PER_PARTITION;
    sb.part_lba  = p->start_sec;

    uint_32 inode_bitmap_len = MFILES_PER_PARTITION/BITS_PER_SECTOR;
    uint_32 inode_array_len = (sizeof(struct inode)*MFILES_PER_PARTITION)/ \
                              BYTES_PER_SECTOR;
    uint_32 free_len = sb.block_cnt - 2 - inode_array_len - inode_bitmap_len;
    uint_32 max_block_bitmap_len = DIV_ROUND_UP(free_len,BITS_PER_SECTOR);
    uint_32 block_bitmap_len = DIV_ROUND_UP(free_len - max_block_bitmap_len, \
                                            BITS_PER_SECTOR);

    sb.block_bitmap_lba  = p->start_sec + 2;
    sb.block_bitmap_sects = block_bitmap_len;
    sb.inode_bitmap_lba  = sb.block_bitmap_lba + sb.block_bitmap_sects;
    sb.inode_bitmap_sects = inode_bitmap_len;
    sb.inode_array_lba   = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
    sb.inode_array_sects  = inode_array_len;
    sb.data_start_lba    = sb.inode_array_lba + sb.inode_array_sects;
    ide_write(p->hd,&sb,sb.part_lba+1,1);

    printk("    magic:0x%x\n    part_lba_base:0x%x\n    all_sector:0x%x\n" \
           "    inode_cnt:0x%x\n    block_bitmap_lba:0x%x\n"  \
           "    block_bitmap_sectors:0x%x\n    inode_bitmap_lba:0x%x\n" \
           "    inode_bitmap_sectors:0x%x\n    inode_table_lba:0x%x\n" \
           "    inode_table_sectors:0x%x\n    data_start_lba:0x%x\n" ,\
           sb.magic,sb.part_lba,sb.block_cnt,sb.inode_cnt,sb.block_bitmap_lba,\
           sb.block_bitmap_sects,sb.inode_bitmap_lba,sb.inode_bitmap_sects, \
           sb.inode_array_lba,sb.inode_array_sects,sb.data_start_lba);

    uint_32 buf_size;
    buf_size = MAX(sb.block_bitmap_sects*512,sb.inode_bitmap_sects*512);
    buf_size = MAX(buf_size,sb.inode_array_sects*512);
    uint_8* buf = sys_malloc(buf_size);

    /* block bitmap */
    buf[0] |= 1;
    uint_32 bit_start = free_len - max_block_bitmap_len;
    uint_32 bit_end   = sb.block_bitmap_sects * BITS_PER_SECTOR - 1;
    if (bit_start < bit_end)
        for (uint_32 idx = bit_start ; idx <= bit_end ; idx++)
        {
            buf[idx/8] |= (1<<(idx%8));
        }
    ide_write(p->hd,buf,sb.block_bitmap_lba,sb.block_bitmap_sects);

    /* inode bitmap */
    memset(buf,0,buf_size);
    buf[0] |= 1;
    ide_write(p->hd,buf,sb.inode_bitmap_lba,sb.inode_bitmap_sects);

    /* inode array:init root inode */
    memset(buf,0,buf_size);
    struct inode* inode_array = (struct inode*)buf;
    inode_array[0].ino = sb.root_inode_no;
    inode_array[0].i_size = 2*sb.dir_entry_size;
    inode_array[0].block[0] = sb.data_start_lba;
    ide_write(p->hd,buf,sb.inode_array_lba,sb.inode_array_sects);

    /* root block data init */
    memset(buf,0,buf_size);
    struct dir_entry* d_ety = (struct dir_entry*)buf;
    // .
    d_ety->ino = sb.root_inode_no;
    strcpy(d_ety->filename,".");
    d_ety->ftype = FT_DIRECTORY;

    // ..
    d_ety++;
    d_ety->ino = sb.root_inode_no;
    strcpy(d_ety->filename,"..");
    d_ety->ftype = FT_DIRECTORY;
    ide_write(p->hd,buf,sb.data_start_lba,1);
    printk("root_dir_lba:0x%x\n",sb.data_start_lba);

    printk("%s format done\n",p->name);
    sys_free(buf);
}


void fs_init(void)
{
    struct super_block* sb = sys_malloc(512);
    if (sb == NULL) {
        PANIC("fs init malloc sb failed");
    }
    printk("searching filesystem...\n");
    for (uint_32 dev_no = 1 ; dev_no < 4 ; dev_no++)
    {
        struct disk* hd = &channel[dev_no/2].disk[dev_no%2];
        struct partition* p = hd->primary;
        for (uint_32 par_no = 0 ; par_no < 11 ; par_no++)
        {
            if (par_no == 4) {
                p = hd->logic;
            }
            if (p->sec_cnt == 0) {
                p++;
                continue;
            }

            ide_read(hd,sb,p->start_sec+1,1);
            if (sb->magic == LINFSMAGIC) {
                printk("%s has filesystem\n",p->name);
            } else {
                printk("formatting %s's partition %s\n",hd->name,p->name);
                fs_format(p);
            }

            memset(sb,0,512);
            p++;
        }
    }
    sys_free(sb);

    char default_part[8] = "sdb1";
    list_traversal(&partition_list,mount_partition,(int)default_part);

    open_root_dir(cur_part);
    for (uint_32 fd_idx = 0 ; fd_idx < MAX_OPEN_FILES ; fd_idx++){
        file_table[fd_idx].inode = NULL;
    }
}

Bool mount_partition(struct list_elm* pt_elm,int arg)
{
    char* part_name = (char*)arg;
    struct partition* pt = mem2entry(struct partition,pt_elm,part_elm);
    if (strcmp(pt->name,part_name)) {
        printk("false:%s\n",pt->name);
        return false;
    }

    cur_part = pt;
    cur_part->sb = sys_malloc(BYTES_PER_SECTOR);    
    if (cur_part->sb == NULL) {
        PANIC("mount_partition alloc memory failed");
    }
    ide_read(pt->hd,cur_part->sb,pt->start_sec+1,1);

    struct super_block* sb = cur_part->sb;
    /* block bitmap */
    cur_part->block_bitmap.map_size = sb->block_bitmap_sects*BYTES_PER_SECTOR;
    cur_part->block_bitmap.bits = sys_malloc(cur_part->block_bitmap.map_size);
    if (cur_part->block_bitmap.bits == NULL) {
        PANIC("mount_partition alloc memory failed");
    }
    ide_read(pt->hd,cur_part->block_bitmap.bits,sb->block_bitmap_lba,sb->block_bitmap_sects);

    /* inode */
    cur_part->inode_bitmap.map_size = sb->inode_bitmap_sects*BYTES_PER_SECTOR;
    cur_part->inode_bitmap.bits = sys_malloc(cur_part->inode_bitmap.map_size);
    if (cur_part->inode_bitmap.bits == NULL) {
        PANIC("mount_partition alloc memory failed");
    }
    ide_read(pt->hd,cur_part->inode_bitmap.bits,sb->inode_bitmap_lba,sb->inode_bitmap_sects);
    list_init(&cur_part->open_inodes);

    printk("mount %s done!\n",cur_part->name);
    return true;
}

int_32 inode_bitmap_alloc(struct partition* part)
{
    struct bitmap* inode_btmp = &part->inode_bitmap;
    uint_32 free_pos = bit_scan(inode_btmp,1);
    if (free_pos == -1) {
        return -1;
    }
    bitmap_set(inode_btmp,free_pos,1);
    return free_pos;
}

int_32 block_bitmap_alloc(struct partition* part)
{
    struct bitmap* block_btmp = &part->block_bitmap;
    struct super_block* sb = part->sb;
    uint_32 sec_off = bit_scan(block_btmp,1);
    if (sec_off == -1) {
        return -1;
    }
    bitmap_set(block_btmp,sec_off,1);
    return sb->data_start_lba + sec_off;
}

void sync_bitmap(struct partition* part,uint_32 bit_idx,uint_8 type)
{
    ASSERT(type==INODE_BITMAP || type==BLOCK_BITMAP);
    struct super_block* sb = part->sb;
    uint_32 lba_off    = bit_idx/8/BYTES_PER_SECTOR;
    uint_32 start_byte = lba_off*BYTES_PER_SECTOR;

    struct bitmap* btmp;
    switch(type){
        case INODE_BITMAP:
            btmp = &part->inode_bitmap;
            ide_write(part->hd,btmp->bits+start_byte,sb->inode_bitmap_lba+lba_off,1);
            break;
        case BLOCK_BITMAP:
            btmp = &part->block_bitmap;
            ide_write(part->hd,btmp->bits+start_byte,sb->block_bitmap_lba+lba_off,1);
            break;
    }
}

const char* path_parse(const char* path,char* name)
{
    const char* pathp = path;
    while (pathp[0] == '/') pathp++;
    while (*pathp && *pathp != '/') (*name++ = *pathp++);
    *name = '\0';
    if (*pathp==NULL || *(pathp+1)==NULL){
        return NULL;
    }
    return pathp;
}

uint_32 path_depth_cnt(const char* path)
{
    ASSERT(path != NULL);
    uint_32 counter = 0;
    char buf[FILENAME_MAXLEN];
    const char* subpath = path;
    while ((subpath = path_parse(subpath,buf))) {
        counter++;
        memset(buf,0,sizeof(buf));
    }
    return counter;
}

int_32 search_file(const char* filename,struct path_search_record* record)
{
    memset(record,0,sizeof(struct path_search_record));
    const char* subpath = filename;
    char cur_name[FILENAME_MAXLEN];
    memset(cur_name,0,sizeof(cur_name));
    struct dir_entry de;
    memset(&de,0,sizeof(de));

    record->p_dir = &root;
    record->ftype = FT_DIRECTORY;
    uint_32 grandfather = record->p_dir->inode->ino;
    char rootdirbuf[512];
    memset(rootdirbuf,0,sizeof(rootdirbuf));
    uint_32 pdirpos = record->p_dir->dir_pos;
    memcpy(rootdirbuf,root.dir_buf,512);
    subpath = path_parse(subpath,cur_name);
    while (*cur_name)
    {
        strcat(record->searched_path,"/");
        strcat(record->searched_path,cur_name);
        if (!search_dir_entry(cur_part,record->p_dir,cur_name,&de)) {
            //printk("search_dir_entry failed\n");
            //printk("searched path:%s\n",record->searched_path);
            return -1;
        }
        record->ftype = de.ftype;
        if (de.ftype == FT_DIRECTORY)
        {
            grandfather = record->p_dir->inode->ino;
            close_dir(record->p_dir);
            record->p_dir = open_dir(cur_part,de.ino);
            memset(cur_name,0,sizeof(cur_name));
            if (subpath) {
                subpath = path_parse(subpath,cur_name);
            }
            continue;
        }
        else if(de.ftype == FT_REGULAR)
        {
            root.dir_pos = pdirpos;
            memcpy(root.dir_buf,rootdirbuf,512);
            return de.ino;
        }
        else
        {
            return -1;
        }
    }

    close_dir(record->p_dir);
    record->p_dir = open_dir(cur_part,grandfather);
    root.dir_pos = pdirpos;
    memcpy(root.dir_buf,rootdirbuf,512);
    return de.ino;
}

int_32 sys_open(const char* filename,uint_8 flag)
{
    if (filename[strlen(filename)-1] == '/') {
        printk("use dir_open to open directory\n");
        return -1;
    }
    struct path_search_record record;
    memset(&record,0,sizeof(record));
    int_32 ino = search_file(filename,&record);
    Bool exist = ino==-1 ? 0 : 1;
    /* remove anormalies */
    if (!exist && !(flag&O_CREAT)) {
        printk("the file you search doesn't exists\n");
        printk("searched path:%s\n",record.searched_path);
        return -1;
    }
    if (exist && record.ftype==FT_DIRECTORY) {
        printk("use dir_open to open directory\n");
        return -1;
    }

    int_32 fd;
    if (exist)
    {
        //printk("file exists,openning now...\n");
        fd = file_open(ino,flag);
    }
    else
    {
        printk("creating new file...\n");
        const char* new_fname = filename;
        if (strrchr(filename,'/') != NULL)
            new_fname = strrchr(filename,'/')+1;
        fd = file_create(record.p_dir,new_fname,flag);
    }
    return fd;
}

int_32 sys_close(int_32 fd)
{
    int_32 ret;
    int_32 _fd = fdlocal2gloabl(fd);
    if (fd >= 3){
        if (is_pipe(fd)) {
            if (--file_table[_fd].fd_pos == 0) {
                mfree_page(PF_KERNEL,file_table[_fd].inode,1);
                file_table[_fd].inode = NULL;
            }
            ret = 0;
        }
        else
            ret = file_close(&file_table[_fd]);
        struct task_struct* cur = running_thread();
        cur->fd_table[fd] = -1;
    }
    return ret;
}

int_32 sys_write(uint_32 fd,const void* buf,uint_32 count)
{
    uint_32 _fd = fdlocal2gloabl(fd);
    if (_fd == -1) {
        printk("can't find file in file table\n");
        return -1;
    }
    if (_fd == stdout) {
        char* buffer = (char*)buf;
        console_put_str(buffer);
        return count;
    }

    if (is_pipe(fd)) {
        return pipe_write(fd,(char*)buf,count);
    }

    struct file* f = &file_table[_fd];
    if (!(f->flag&O_WRONLY || f->flag&O_RDWT)) {
        printk("can't write without write flag\n");
        return -1;
    }
    uint_32 written = file_write(f,buf,count);
    return written;
}

int_32 sys_read(uint_32 fd,void* buf,uint_32 count)
{
    ASSERT(buf != NULL);
    if (fd == stdout || fd == stderr) {
        printk("sys_read: wrong fd\n");
        return -1;
    }

    uint_32 _fd = fdlocal2gloabl(fd);
    int_32 ret = -1;
    if (_fd == stdin)
    {
        char* buffer = buf;
        uint_32 bytes_read = 0;
        while(count--) {
            *buffer++ = ioq_getchar(&kbd_buf);
            bytes_read++;
        }
        ret = count - bytes_read;
    }
    else if (is_pipe(fd))
    {
        pipe_read(fd,buf,count);
    }
    else
    {
        if (_fd == -1) {
            printk("can't find file in file table\n");
            return -1;
        }
        struct file* f = &file_table[_fd];
        if (!(f->flag==O_RDONLY || f->flag&O_RDWT)) {
            printk("can't read without read flag\n");
            return -1;
        }
        ret = file_read(f,buf,count);
    }
    return ret;
}

int_32 sys_lseek(uint_32 fd,int_32 offset,uint_8 whence)
{
    ASSERT(whence>0 && whence<4);
    if (fd < 0) {
        printk("sys_lseek: fd < 0\n");
        return -1;
    }
    struct task_struct* cur = running_thread();
    uint_32 _fd = fdlocal2gloabl(fd);
    struct file* f = &file_table[_fd];
    int_32 new_pos;
    switch(whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = (int_32)f->fd_pos + offset;
            break;
        case SEEK_END:
            new_pos = f->inode->i_size + offset;
            break;
    }
    if (new_pos > f->inode->i_size || new_pos < 0) {
        printk("exceed the range of file\n");
        return -1;
    }
    f->fd_pos = new_pos;
    return f->fd_pos;
}

int_32 sys_unlink(const char* filename)
{
    struct path_search_record record;
    int_32 ino = search_file(filename,&record);
    if (ino == -1){
        printk("can't find the file\n");
        return -1;
    }
    if (record.ftype == FT_DIRECTORY) {
        printk("can't remove a directory,use ...(name forget) instead\n");
        return -1;
    }

    Bool file_open = false;
    for (uint_32 idx = 0 ; idx < MAX_OPEN_FILES ; idx++)
    {
        struct file* f = &file_table[idx];
        if (f->inode != NULL && f->inode->ino == ino){
            file_open = true;
            break;
        }
    }
    if (file_open) {
        printk("can't close the file that in use\n");
        return -1;
    }

    void* io_buf = sys_malloc(BLOCKSIZE*2);
    delete_dir_entry(cur_part,record.p_dir,ino,io_buf);
    inode_release(cur_part,ino);
    sys_free(io_buf);
    close_dir(record.p_dir);
    return 0;
}

int_32 sys_mkdir(const char* pathname)
{
    void* buf = sys_malloc(BLOCKSIZE*2); //惨痛经历：只因sync_dir_entry 用尺寸1024
    if (buf == NULL) {
        printk("sys_mkdir:buf malloc failed\n");
        return -1;
    }
    memset(buf,0,BLOCKSIZE);

    struct path_search_record record;
    int_32 ino = search_file(pathname,&record);
    if (ino != -1) {
        printk("the dir already exists\n");
        return -1;
    }
    uint_32 depth = path_depth_cnt(pathname);
    if (depth != path_depth_cnt(record.searched_path)) {
        printk("lack some dir,current at:%s\n",record.searched_path);
        return -1;
    }

    struct inode inode;
    ino = inode_bitmap_alloc(cur_part);
    if (ino == -1) {
        printk("sys_mkdir:inode_bitmap alloc failed\n");
        return -1;
    }
    inode_init(&inode,ino);

    struct dir_entry de;
    memset(&de,0,sizeof(de));
    const char* dirname = strrchr(pathname,'/')+1;
    dir_entry_init(&de,ino,dirname,FT_DIRECTORY);
    if (!sync_dir_entry(record.p_dir,&de,buf)) {
        printk("sys_mkdir:sync_dir_entry failed\n");
        return -1;
    }
    memset(buf,0,BLOCKSIZE);

    uint_32 first_lba= block_bitmap_alloc(cur_part);
    if (first_lba == -1) {
        printk("sys_malloc:no avaliable block\n");
        return -1;
    }
    sync_bitmap(cur_part,ino,INODE_BITMAP);

    uint_32 blk_idx = first_lba - cur_part->sb->data_start_lba;
    bitmap_set(&cur_part->block_bitmap,blk_idx,1);
    sync_bitmap(cur_part,blk_idx,BLOCK_BITMAP);
    inode.block[0] = first_lba;

    struct dir_entry* d_ety = (struct dir_entry*)buf;
    // .
    d_ety->ino = ino;
    strcpy(d_ety->filename,".");
    d_ety->ftype = FT_DIRECTORY;
    inode.i_size += cur_part->sb->dir_entry_size;
    // ..
    d_ety++;
    d_ety->ino = record.p_dir->inode->ino;
    strcpy(d_ety->filename,"..");
    d_ety->ftype = FT_DIRECTORY;
    inode.i_size += cur_part->sb->dir_entry_size;
    ide_write(cur_part->hd,buf,first_lba,1);
    memset(buf,0,BLOCKSIZE);
    sync_inode_array(cur_part,&inode,buf);

    close_dir(record.p_dir);
    sys_free(buf);
    return 0;
}

struct dir* sys_opendir(const char* pathname)
{
    if (pathname[0]=='/' && (pathname[1]==0 || pathname[1]=='.')) {
        // 真是个自欺欺人的简陋实现
        open_dir(cur_part,0);
        return &root;
    }
    struct path_search_record record;
    memset(&record,0,sizeof(record));
    struct dir* ret = NULL;
    int_32 dir_ino = search_file(pathname,&record);
    if (dir_ino == -1) {
        printk("can't find dir you want to open\n");
        printk("searched_path:%s/n",record.searched_path);
        return NULL;
    }
    if (record.ftype != FT_DIRECTORY) {
        printk("opendir only open dir\n");
        return NULL;
    }

    ret = open_dir(cur_part,dir_ino);
    close_dir(record.p_dir);
    return ret;
}

int_32 sys_closedir(struct dir* dir)
{
    int_32 ret = -1;
    if (dir != NULL) {
        close_dir(dir);
        ret = 0;
    }
    return ret;
}

struct dir_entry* sys_readdir(struct dir* dir) {
    ASSERT(dir!=NULL);
    return dir_read(dir);
}

void sys_rewinddir(struct dir* dir) {
    dir->dir_pos = 0;
}

int_32 sys_rmdir(const char* pathname)
{
    struct path_search_record record;
    int_32 ino = search_file(pathname,&record);
    if (ino == -1) {
        printk("sys_rmdir: can't find dir you want to delete\n");
        return -1;
    }
    if (record.ftype != FT_DIRECTORY) {
        printk("sys_rmdir: only remove empty directory\n");
        return -1;
    }

    struct dir* cur_dir = open_dir(cur_part,ino);
    ASSERT(cur_dir != NULL);
    if (!dir_empty(cur_part,cur_dir)) {
        printk("sys_rmdir:the dir is not empty\n");
        return -1;
    }

    int_32 ret = dir_remove(record.p_dir,cur_dir);

    close_dir(record.p_dir);
    return ret;
}

int_32 sys_getcwd(char* buf,uint_32 size)
{
    ASSERT(buf != NULL);
    memset(buf,0,size);
    void* io_buf = sys_malloc(BLOCKSIZE);
    if (io_buf == NULL) {
        printk("sys_getcwd: io_buf malloc failed\n");
        return -1;
    }

    struct task_struct* cur = running_thread();
    uint_32 cino=cur->cwd_inode_nr,pino=0;
    if (cino == 0) {
        buf[0] = '/';
        buf[1] = 0;
        return 0;
    }
    char reverse_path[MAX_PATH_LEN];
    memset(reverse_path,0,sizeof(reverse_path));
    while((cino))
    {
        pino = get_parent_inode_nr(cur_part,cino,io_buf);
        memset(io_buf,0,BLOCKSIZE);
        get_child_dirname(cur_part,pino,cino,reverse_path,io_buf);
        cino = pino;
    }
    if (strlen(reverse_path)+1 >= size) {
        printk("out of bufsize\n");
        return -1;
    }

    char* dirname;
    while ((dirname = strrchr(reverse_path,'/')) != NULL)
    {
        strcat(buf+strlen(buf),dirname);
        *dirname = 0;
    }

    sys_free(io_buf);
    return 0;
}

int_32 sys_chdir(const char* path)
{
    struct path_search_record record;
    int_32 dino = search_file(path,&record);
    if (dino == -1) {
        close_dir(record.p_dir);
        printk("sys_chdir: no such directory\n");
        return -1;
    }
    if (record.ftype != FT_DIRECTORY) {
        close_dir(record.p_dir);
        printk("sys_chdir: is a file\n");
        return -1;
    }

    struct task_struct* cur = running_thread();
    cur->cwd_inode_nr = dino;
    close_dir(record.p_dir);
    return 0;
}

int_32 sys_stat(const char* path,struct stat* fstat)
{
    struct path_search_record record;
    int_32 ino = search_file(path,&record);
    if (ino == -1) {
        printk("sys_stat: search_file failed\n");
        return -1;
    }

    struct inode* inode = inode_open(cur_part,ino);
    fstat->st_ino   = ino;
    fstat->st_fsize = inode->i_size;
    fstat->st_ftype = record.ftype;

    close_dir(record.p_dir);
    return 0;
}
void sys_help(void) {
    printk("buildin commands:\n\
           ls: show directory or file information\n\
           cd: change current work directory\n\
           mkdir: create a directory\n\
           rmdir: remove a empty directory\n\
           rm: remove a regular file\n\
           pwd: show current work directory\n\
           ps: show process information\n\
           clear: clear screen\n\
           shortcut key:\n\
           ctrl+l: clear screen\n\
           ctrl+u: clear input\n\n");
}
