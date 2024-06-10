#include "syscall.h"
#include "stdio.h"
#include "fs.h"
#include "file.h"

int main(int argc,char* argv[])
{
    if (argc > 2) {
        printf("usage:cat or cat [file]");
        exit(-2);
    }

    if (argc == 1) {
        char buf[512];
        memset(buf,0,sizeof(buf));
        read(0,buf,512);
        printf("%s",buf);
        exit(0);
    }

    char abs_path[MAX_PATH_LEN];
    if(argv[1][0]!='/') {
        getcwd(abs_path,MAX_PATH_LEN);
        strcat(abs_path,"/");
        strcat(abs_path,argv[1]);
    } else {
        strcpy(abs_path,argv[1]);
    }

    char buf[256];
    int read_bytes;
    int fd = open(abs_path,O_RDONLY);
    if (fd == -1) {
        printf("cat open failed");
        return -1;
    }
    while(1) {
        memset(buf,0,sizeof(buf));
        read_bytes = read(fd,buf,256);
        if (read_bytes == -1) {
            break;
        }
        write(1,buf,256);
    }
    close(fd);
    return 66;
}
