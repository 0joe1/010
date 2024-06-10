#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

#define PANIC(...) panic_spin(__FILE__ , __LINE__ , __func__ , __VA_ARGS__)

void panic_spin(char* file,int line,const char* func,const char* condition);

#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0)
#else
    #define ASSERT(CONDITION) if(CONDITION){} \
                              else PANIC(#CONDITION);
#endif

#endif
