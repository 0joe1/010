#include "syscall.h"
#include "stdio.h"

int main(int argc,char *argv[])
{
    int fd[2];
    printf("pipe start\n");
    pipe(fd);
    int pid = fork();
    if (pid) {
        printf("father start\n");
        close(fd[0]);
        write(fd[1],"pipe done",10);
        printf("father exit\n");
        return 9;
    }
    else
    {
        printf("child start\n");
        close(fd[1]);
        char buf[32];
        memset(buf,0,sizeof(buf));
        read(fd[0],buf,10);
        printf("father said %s\n",buf);
        printf("child exit\n");
        return 7;
    }
}
