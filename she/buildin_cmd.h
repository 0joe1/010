#ifndef SHELL_BUILDIN_CMD_H
#define SHELL_BUILDIN_CMD_H
#include "stdio.h"

static void wash_path(char* old_path,char* newpath);
void make_abs_path(char* path,char* clr_path);
void buildin_pwd(uint_32 argc,char** argv);
int_32 buildin_mkdir(uint_32 argc,char** argv);
int_32 buildin_cd(uint_32 argc,char** argv);
int_32 buildin_rmdir(uint_32 argc,char** argv);
int_32 buildin_ls(uint_32 argc,char** argv);
int_32 buildin_ps(uint_32 argc,char** argv);
int_32 buildin_clear(uint_32 argc,char** argv);
int_32 buildin_rm(uint_32 argc,char** argv);

#endif
