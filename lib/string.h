#ifndef LIB_STRING_H
#define LIB_STRING_H
#include "stdint.h"

#define NULL 0
void memset(void* dst_,uint_8 value,uint_32 size);
void memcpy(void* dst_,void* src_,uint_32 size);
int memcmp(const void* a_,const void* b_,uint_32 size);

char* strcpy(char* dst_,const char* src_);
uint_32 strlen(const char* str);
int_8 strcmp(const char* a,const char* b);
char* strchr(const char* str,uint_8 ch);
char* strrchr(const char* str,char ch);
char* strcat(char* dst_,const char* src_);
uint_32 strchrs(const char* str,uint_8 ch);


#endif
