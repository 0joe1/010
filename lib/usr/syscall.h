#ifndef LIB_USR_SYSCALL_H
#define LIB_USR_SYSCALL_H
#include "stdint.h"
#include "thread.h"
#include "fs.h"

enum SYSCALL {
    SYS_GETPID,
    SYS_WRITE,
    SYS_READ,
    SYS_MALLOC,
    SYS_FREE,
    SYS_FORK,
    SYS_PUTCHAR,
    SYS_CLS,
    SYS_GETCWD,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_LSEEK,
    SYS_UNLINK,
    SYS_MKDIR,
    SYS_OPENDIR,
    SYS_CLOSEDIR,
    SYS_CHDIR,
    SYS_RMDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    SYS_STAT,
    SYS_PS,
    SYS_EXECV,
    SYS_EXIT,
    SYS_WAIT,
    SYS_PIPE,
    SYS_FDREDIRECT,
    SYS_HELP
};

uint_32 getpid(void);
int_32 write(uint_32 fd,const void* buf,uint_32 count);
int_32 read(uint_32 fd,void* buf,uint_32 count);
void* malloc(uint_32 size);
void free(void* ptr);
pid_t fork(void);
void putchar(char c);
void clear(void);
int_32 getcwd(char* buf,uint_32 size);
int_32 open(const char* filename,uint_8 flag);
int_32 close(int_32 fd);
int_32 lseek(uint_32 fd,int_32 offset,uint_8 whence);
int_32 unlink(const char* filename);
int_32 mkdir(const char* pathname);
int_32 rmdir(const char* pathname);
struct dir* opendir(const char* pathname);
int_32 closedir(struct dir* dir);
int_32 chdir(const char* path);
struct dir_entry* readdir(struct dir* dir);
void rewinddir(struct dir* dir);
int_32 stat(const char* path,struct stat* fstat);
void ps(void);
int_32 execv(const char* pathname,char** argv);
void exit(int_32 status);
pid_t wait(int_32* status);
int_32 pipe(int p[]);
void fd_redirect(uint_32 old_fd,uint_32 new_fd);
void help(void);

#endif
