#include "console.h"
#include "sync.h"
#include "print.h"
#include "interrupt.h"

static struct lock console_lock;

void console_init() {
    lock_init(&console_lock);
}

void console_put_str(char* str)
{
    lock_acquire(&console_lock);
    put_str(str);
    lock_release(&console_lock);
}

void console_put_char(uint_8 c)
{
    lock_acquire(&console_lock);
    put_char(c);
    lock_release(&console_lock);
}

void console_put_int(uint_32 num)
{
    lock_acquire(&console_lock);
    put_int(num);
    lock_release(&console_lock);
}

