#include "stdio-kernel.h"
#include "console.h"
#include "stdio.h"

void printk(const char* format,...) 
{
    va_list ap;
    va_start(ap,format);
    char buf[1024]={0};
    vsprintf(buf,format,ap);
    va_end(ap);
    console_put_str(buf);
}
