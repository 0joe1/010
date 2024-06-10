#include "pipe.h"
#include "ioqueue.h"
#include "file.h"
#include "debug.h"

#define TYPE_PIPE 0xffff

int sys_pipe(int p[])
{
    int _fd = get_free_slot_filetable();
    file_table[_fd].inode = get_kernel_pages(1);
    file_table[_fd].flag  = TYPE_PIPE;
    ioq_init((struct ioqueue*)file_table[_fd].inode);

    file_table[_fd].fd_pos = 2;
    p[0] = pcb_fd_install(_fd);
    p[1] = pcb_fd_install(_fd);

    return 0;
}

int is_pipe(int fd)
{
    int _fd = fdlocal2gloabl(fd);
    return (file_table[_fd].flag == TYPE_PIPE);
}

int pipe_read(int fd,char* buf,int count)
{
    int read_bytes = 0,tot_read;
    int _fd = fdlocal2gloabl(fd);
    struct ioqueue *ioq = (struct ioqueue*)file_table[_fd].inode;
    int remain = ioq_len(ioq);
    tot_read = (count < remain ? count : remain);

    while (read_bytes < tot_read) {
        buf[read_bytes++] = ioq_getchar(ioq);
    }
    ASSERT(read_bytes == tot_read);

    return read_bytes;
}

int pipe_write(int fd,char* buf,int count)
{
    int write_bytes = 0;
    int _fd = fdlocal2gloabl(fd);
    struct ioqueue *ioq = (struct ioqueue*)file_table[_fd].inode;
    int remain = BUFSIZE - ioq_len(ioq) - 1;
    int tot_write = (count < remain ? count : remain);

    while (write_bytes < tot_write) {
        ioq_putchar(ioq,buf[write_bytes++]);
    }
    ASSERT(write_bytes == tot_write);

    return write_bytes;
}

void sys_fd_redirect(uint_32 old_fd,uint_32 new_fd)
{
    struct task_struct *cur = running_thread();
    if (new_fd < 3) {
        cur->fd_table[old_fd] = new_fd;
    } else {
        uint_32 _fd = cur->fd_table[new_fd];
        cur->fd_table[old_fd] = _fd;
    }
}

