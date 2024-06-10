#include "interrupt.h"
#include "timer.h"
#include "print.h"
#include "init.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "global.h" 
#include "tss.h"
#include "syscall-init.h"
#include "ide.h"
#include "fs.h"

void init_all()
{
    put_str("init all\n");
    idt_init();
    mem_init();
    timer_init();
    keyboard_init();
    tss_init();
    syscall_init();
    thread_init();
    console_init();
    ide_init();
    fs_init();
}
