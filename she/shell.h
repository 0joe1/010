#ifndef SHELL_H
#define SHELL_H
#include "stdint.h"

#define cmd_len 128
#define MAX_ARG_NR 7

extern char cwd_cache[];

Bool readline(char* buf,uint_32 count);
void print_prompt(void);
static int_32 cmd_parse(char* cmd_str,char** argv,char tocken);
void my_shell(void);

#endif
