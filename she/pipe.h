#ifndef __PIPE_H
#define __PIPE_H
#include "stdint.h"

int is_pipe(int fd);
int sys_pipe(int p[]);
int pipe_read(int fd,char* buf,int count);
int pipe_write(int fd,char* buf,int count);
void sys_fd_redirect(uint_32 old_fd,uint_32 new_fd);

#endif
