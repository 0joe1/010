#include "debug.h"
#include "print.h"
#include "interrupt.h"

void panic_spin(char* file,int line,const char* func,const char* condition)
{
    intr_disable();
    put_str("\n\n\n !!!error!!! \n");
    put_str("filename: ");put_str(file);put_char('\n');
    put_str("line: ");put_int(line);put_char('\n');
    put_str("function: ");put_str((char*)func);put_char('\n');
    put_str("condition: ");put_str((char*)condition);put_char('\n');
    while(1);   //make it spinning
}
