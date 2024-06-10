#ifndef LIB_STDIO_H
#define LIB_STDIO_H
#include "stdint.h"
#include "string.h"

#define _INTSIZEOF(n)  ((sizeof(n) + sizeof(int)-1) & ~(sizeof(int)-1))
#define va_list char*
#define va_start(ap,v) ( ap = (va_list)&v + _INTSIZEOF(v) )
#define va_arg(ap,T) ( *(T*)((ap += _INTSIZEOF(T)) - _INTSIZEOF(T)) )
#define va_end(ap)  ( ap = NULL )

uint_32 vsprintf(char* buf,const char* format,va_list ap);
uint_32 printf(const char* format,...);
uint_32  sprintf(char* buf,const char* format,...);

#endif
