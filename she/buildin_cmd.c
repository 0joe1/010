#include "buildin_cmd.h"
#include "string.h"
#include "shell.h"
#include "fs.h"
#include "dir.h"
#include "print.h"
#include "syscall.h"

static void wash_path(char* old_path,char* newpath)
{
    if (old_path == NULL) {
        return ;
    }
    newpath[0] = '/';

    char name[FILENAME_MAXLEN];
    memset(name,0,sizeof(name));
    const char* subpath = path_parse(old_path,name);
    char* splash;
    while (*name)
    {
        if (!strcmp(name,"..")) {
            splash = strrchr(newpath,'/');
            if (splash == newpath) {
                *(splash+1) = 0;
            } else {
                *splash = 0;
            }
        }
        else if (strcmp(name,".")) {
            if (newpath[strlen(newpath) -1] != '/')
                strcat(newpath,"/");
            strcat(newpath,name);
        }
        memset(name,0,sizeof(name));
        if (subpath) {
            subpath = path_parse(subpath,(char*)name);
        }
    }
}

static void trip_backsplash(char* path) {
    while (strlen(path)) {
        if (strlen(path)==1 && path[0]=='/') return ;

        if (path[strlen(path)-1] == '/')
            path[strlen(path)-1] = 0;
        else
            return ;
    }
}

void make_abs_path(char* path,char* clr_path)
{
    trip_backsplash(path);
    char buf[MAX_PATH_LEN];
    memset(buf,0,sizeof(buf));
    if (path[0] != '/') {
        strcpy(buf,cwd_cache);
        strcat(buf,"/");
    }
    strcat(buf,path);
    wash_path(buf,clr_path);
}

void buildin_pwd(uint_32 argc,char** argv)
{
    if (argc !=1) {
        printf("pwd:no argument\n");
        return ;
    }
    printf("%s\n",cwd_cache);
}

int_32 buildin_mkdir(uint_32 argc,char** argv)
{
    if (argc < 2) {
        printf("mkdir file1 file2 ...\n");
        return -1;
    }

    char abs_path[MAX_PATH_LEN];
    memset(abs_path,0,sizeof(abs_path));
    for (uint_32 ag = 1 ; ag < argc ; ag++)
    {
        make_abs_path(argv[ag],abs_path);
        if (!strcmp(abs_path,"/")) {
            return -1;
        }
        if (mkdir(abs_path) == -1){
            return -1;
        }
    }
    return 0;
}

int_32 buildin_cd(uint_32 argc,char** argv)
{
    if (argc != 2) {
        printf("cd dir\n");
        return -1;
    }

    char final_path[MAX_PATH_LEN];
    memset(final_path,0,sizeof(final_path));
    make_abs_path(argv[1],final_path);
    if (chdir(final_path) == -1) {
        return -1;
    }
    memset(cwd_cache,0,MAX_PATH_LEN);
    memcpy(cwd_cache,final_path,MAX_PATH_LEN);

    return 0;
}

int_32 buildin_rmdir(uint_32 argc,char** argv)
{
    if (argc < 2) {
        printf("rmdir dir1 dir2 ...\n");
        return -1;
    }

    if (!strcmp(argv[1],"/")) {
        printf("root can't be removed\n");
        return -1;
    }

    char final_path[MAX_PATH_LEN];
    memset(final_path,0,sizeof(final_path));
    make_abs_path(argv[1],final_path);
    if (rmdir(final_path) == -1) {
        return -1;
    }

    return 0;
}

int_32 buildin_ls(uint_32 argc,char** argv)
{
    if (argc > 3) {
        printf("Now only support 'ls -l file'");
        return -1;
    }
    char abs_path[MAX_PATH_LEN];
    char abs_bak[MAX_PATH_LEN];
    Bool longinfo = false;
    memset(abs_path,0,sizeof(abs_path));

    for (uint_32 ag = 1 ; ag < argc ; ag++) {
        if (argv[ag][0] == '-') longinfo=true;
        else make_abs_path(argv[ag],abs_path);
    }
    if (argc == 1 || (argc==2 && longinfo==true)) {
        memcpy(abs_path,cwd_cache,MAX_PATH_LEN);
        strcpy(abs_bak,abs_path);
    }

    struct stat fstat;
    if (stat(abs_path,&fstat) == -1) {
        printf("ls: no such file or directory!\n");
        return -1;
    }

    char buf[16];
    char ftype;
    if (fstat.st_ftype == FT_DIRECTORY)
    {
        if (fstat.st_ino != 0) strcat(abs_path,"/");
        struct dir_entry* de = NULL;
        struct dir* dir = opendir(abs_path);
        if (dir == NULL) {
            return -1;
        }
        if (longinfo)
        {
            while ((de = readdir(dir)) != NULL)
            {
                strcat(abs_path,"/");
                strcat(abs_path,de->filename);
                stat(abs_path,&fstat);
                ftype = (fstat.st_ftype==FT_REGULAR?'-':'d');
                pad_print(buf,10,&ftype,'c');
                pad_print(buf,10,&fstat.st_ino,'d');
                pad_print(buf,10,&fstat.st_fsize,'d');
                printf("%s\n",de->filename);
                strcpy(abs_path,abs_bak);
            }
        }
        else
        {
            while ((de = readdir(dir)) != NULL){
                printf("%s  ",de->filename);
            }
            printf("\n");
        }
        closedir(dir);
        return 0;
    }

    ftype = 'd';
    pad_print(buf,16,&ftype,'c');
    pad_print(buf,16,&fstat.st_ino,'d');
    pad_print(buf,16,&fstat.st_fsize,'d');
    printf("\n");

    return 0;
}

int_32 buildin_ps(uint_32 argc,char** argv)
{
    if (argc != 1) {
        printf("ps: no argument\n");
        return -1;
    }
    ps();
    return 0;
}

int_32 buildin_clear(uint_32 argc,char** argv)
{
    if (argc != 1) {
        printf("clear: no argument\n");
        return -1;
    }
    clear();
    return 0;
}

int_32 buildin_rm(uint_32 argc,char** argv)
{
    if (argc < 2) {
        printf("format: rm file1 file2 ...");
        return -1;
    }

    char abs_path[MAX_PATH_LEN];
    memset(abs_path,0,sizeof(abs_path));
    struct stat fstat;
    for (uint_32 ag = 1 ; ag < argc ; ag++)
    {
        make_abs_path(argv[ag],abs_path);
        stat(abs_path,&fstat);
        if (fstat.st_ftype == FT_DIRECTORY) {
            printf("can't remove directory\n");
            return -1;
        }
        unlink(abs_path);
    }
    return 0;
}





