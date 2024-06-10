#include "assert.h"
#include "stdio.h"

void user_panic(char* file,int line,const char* func,const char* condition)
{
    printf("\n\n\n !!!error!!! \n");
    printf("filename: %s\n",file);
    printf("line: %d\n",line);
    printf("function: %s\n",func);
    printf("condition: %s\n",condition);
    while(1);   //make it spinning
}
