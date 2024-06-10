#ifndef LIB_USR_ASSERT
#define LIB_USR_ASSERT

#define panic(...) user_panic(__FILE__ , __LINE__ , __func__ , __VA_ARGS__)

void user_panic(char* file,int line,const char* func,const char* condition);

#ifdef NDEBUG
    #define assert(CONDITION) ((void)0)
#else
    #define assert(CONDITION) if(CONDITION){} \
                              else panic(#CONDITION);
#endif

#endif
