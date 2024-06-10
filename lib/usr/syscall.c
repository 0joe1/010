#include "syscall.h"
#include "console.h"
#include "string.h"
#include "stdint.h"

#define _syscall0(NUMBER) ({\
        int retval;         \
        asm volatile(       \
        "int $0x80;"        \
        : "=a"(retval)      \
        : "0"(NUMBER)       \
        );                  \
        retval;             \
    })

#define _syscall1(NUMBER,ARG1) ({       \
        int retval;                     \
        asm volatile(                   \
        "int $0x80;"                    \
        : "=a"(retval)                  \
        : "0"(NUMBER),"b"(ARG1)         \
        );                              \
        retval;                         \
    })

#define _syscall2(NUMBER,ARG1,ARG2) ({       \
        int retval;                     \
        asm volatile(                   \
        "int $0x80;"                    \
        : "=a"(retval)                  \
        : "0"(NUMBER),"b"(ARG1),"c"(ARG2)   \
        );                              \
        retval;                         \
    })

#define _syscall3(NUMBER,ARG1,ARG2,ARG3) ({       \
        int retval;                     \
        asm volatile(                   \
        "int $0x80;"                    \
        : "=a"(retval)                  \
        : "0"(NUMBER),"b"(ARG1),"c"(ARG2),"d"(ARG3) \
        );                              \
        retval;                         \
    })

uint_32 getpid(void) {
    return _syscall0(SYS_GETPID);
}

int_32 read(uint_32 fd,void* buf,uint_32 count) {
    return _syscall3(SYS_READ,fd,buf,count);
}

int_32 write(uint_32 fd,const void* buf,uint_32 count) {
    return _syscall3(SYS_WRITE,fd,buf,count);
}

void* malloc(uint_32 size) {
    return (void*)_syscall1(SYS_MALLOC,size);
}

void free(void* ptr) {
    _syscall1(SYS_FREE,ptr);
}

pid_t fork(void) {
    return _syscall0(SYS_FORK);
}

void putchar(char c) {
    _syscall1(SYS_PUTCHAR,c);
}

void clear(void) {
    _syscall0(SYS_CLS);
}

int_32 getcwd(char* buf,uint_32 size) {
    return _syscall2(SYS_GETCWD,buf,size);
}

int_32 open(const char* filename,uint_8 flag) {
    return _syscall2(SYS_OPEN,filename,flag);
}

int_32 close(int_32 fd) {
    return _syscall1(SYS_CLOSE,fd);
}

int_32 lseek(uint_32 fd,int_32 offset,uint_8 whence) {
    return _syscall3(SYS_LSEEK,fd,offset,whence);
}

int_32 unlink(const char* filename) {
    return _syscall1(SYS_UNLINK,filename);
}

int_32 mkdir(const char* pathname) {
    return _syscall1(SYS_MKDIR,pathname);
}

struct dir* opendir(const char* pathname) {
    return (struct dir*)_syscall1(SYS_OPENDIR,pathname);
}

int_32 chdir(const char* path) {
    return _syscall1(SYS_CHDIR,path);
}

int_32 rmdir(const char* pathname) {
    return _syscall1(SYS_RMDIR,pathname);
}

struct dir_entry* readdir(struct dir* dir) {
    return (struct dir_entry*)_syscall1(SYS_READDIR,dir);
}

void rewinddir(struct dir* dir) {
    _syscall1(SYS_REWINDDIR,dir);
}

int_32 stat(const char* path,struct stat* fstat) {
    return _syscall2(SYS_STAT,path,fstat);
}

void ps(void) {
    _syscall0(SYS_PS);
}

int_32 closedir(struct dir* dir) {
    return _syscall1(SYS_CLOSEDIR,dir);
}

int_32 execv(const char* pathname,char** argv) {
    return _syscall2(SYS_EXECV,pathname,argv);
}

void exit(int_32 status) {
    _syscall1(SYS_EXIT,status);
}

pid_t wait(int_32* status) {
    return _syscall1(SYS_WAIT,status);
}

int_32 pipe(int p[]) {
    return _syscall1(SYS_PIPE,p);
}

void fd_redirect(uint_32 old_fd,uint_32 new_fd) {
    _syscall2(SYS_FDREDIRECT,old_fd,new_fd);
}

void help(void) {
    _syscall0(SYS_HELP);
}

