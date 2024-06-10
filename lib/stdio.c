#include "stdio.h"
#include "debug.h"
#include "string.h"
#include "syscall.h"
#include "assert.h"

static void itoa(uint_32 val,char** buf_ptr,uint_8 base)
{
    assert(base<=16);
    uint_32 m = val % base;
    uint_32 q = val / base;
    if (q) {
        itoa(q,buf_ptr,base);
    }
    char c;
    if (m>=0 && m<=9) {
        c = m + '0';
    } else if (m>=10 && m<=15) {
        c = m-10 + 'a';
    }
    *((*buf_ptr)++) = c;
}

uint_32 vsprintf(char* buf,const char* format,va_list ap)
{
    const char* fmtp = format;
    char* bufp = buf;

    uint_32 val;
    char* arg_str;
    while(*fmtp)
    {
        if (*fmtp != '%'){
            *bufp = *(fmtp++);
            ++bufp;
            continue;
        }
        char next_char = *(++fmtp);
        switch(next_char)
        {
            case 'x':
                val = va_arg(ap,int);
                itoa(val,&bufp,16);
                ++fmtp;
                break;
            case 'd':
                val = va_arg(ap,int);
                if (val < 0) {
                    *(bufp++) = '-';
                    val = -val;
                }
                itoa(val,&bufp,10);
                ++fmtp;
                break;
            case 'c':
                *(bufp++) = va_arg(ap,char);
                ++fmtp;
                break;
            case 's':
                arg_str = va_arg(ap,char*);
                strcpy(bufp,arg_str);
                bufp += strlen(arg_str);
                ++fmtp;
                break;
        }
    }

    return strlen(buf);
}

uint_32 printf(const char* format,...)
{
    char buf[1024]={0};
    uint_32 retLen;
    va_list ap;
    va_start(ap,format);
    vsprintf(buf,format,ap);
    va_end(ap);
    retLen = write(1,buf,strlen(buf));
    return retLen;
}

uint_32 sprintf(char* buf,const char* format,...)
{
    uint_32 retLen;
    va_list ap;
    va_start(ap,format);
    retLen = vsprintf(buf,format,ap);
    buf[retLen] = '\0';
    va_end(ap);
    return retLen;
}
