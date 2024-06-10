#include "keyboard.h"
#include "print.h"
#include "interrupt.h"
#include "io.h"
#include "ioqueue.h"
#include "syscall.h"

#define KDB_BUF_PORT 0x60
#define IS_DOUBLECHAR(code) ((code<0x0e)||(code==0x29)||(code==0x1a)||(code==0x1b)||(code==0x2b)||(code==0x33)||(code==0x27)||(code==0x28)||(code==0x34)||(code==0x35))

struct ioqueue kbd_buf;

static Bool ctrl_l_status,ctrl_r_status,ctrl_status;
static Bool shift_l_status,shift_r_status,shift_status;
static Bool alt_l_status,alt_r_status,alt_status;
static Bool ext_status,caps_lock_status;

static char keymap[][2]= {
    {0,0},
    {esc,esc},
    {'1','!'},
    {'2','@'},
    {'3','#'},
    {'4','$'},
    {'5','%'},
    {'6','^'},
    {'7','&'},
    {'8','*'},
    {'9','('},
    {'0',')'},
    {'-','_'},
    {'=','+'},
    {backspace,backspace},
    {tab,tab},
    {'q','Q'},
    {'w','W'},
    {'e','E'},
    {'r','R'},
    {'t','T'},
    {'y','Y'},
    {'u','U'},
    {'i','I'},
    {'o','O'},
    {'p','P'},
    {'[','{'},
    {']','}'},
    {enter,enter},
    {ctrl_l_char,ctrl_l_char},
    {'a','A'},
    {'s','S'},
    {'d','D'},
    {'f','F'},
    {'g','G'},
    {'h','H'},
    {'j','J'},
    {'k','K'},
    {'l','L'},
    {';',':'},
    {'\'','"'},
    {'`','~'},
    {shift_l_char,shift_r_char},
    {'\\','|'},
    {'z','Z'},
    {'x','X'},
    {'c','C'},
    {'v','V'},
    {'b','B'},
    {'n','N'},
    {'m','M'},
    {',','<'},
    {'.','>'},
    {'/','?'},
    {shift_r_char,shift_r_char},
    {'*','*'},
    {alt_l_char,alt_r_char},
    {' ',' '},
    {caps_lock_char,caps_lock_char},
};

static void code_swistat(uint_16 code,uint_8 val)
{
    if      (code == ctrl_l_make)  ctrl_l_status  = val;
    else if (code == ctrl_r_make)  ctrl_r_status  = val;
    else if (code == shift_l_make) shift_l_status = val;
    else if (code == shift_r_make) shift_r_status = val;
    else if (code == alt_l_make)   alt_l_status   = val;
    else if (code == alt_r_make)   alt_r_status   = val;
}
static void updatestat(void)
{
    ctrl_status  = ctrl_l_status | ctrl_r_status;
    shift_status = shift_l_status | shift_r_status;
    alt_l_status = alt_l_status | alt_r_status;
}

static void intr_keyboard_handler(void)
{
    uint_16 scancode;
    scancode = inb(KDB_BUF_PORT);
    if (scancode == 0xe0){
        ext_status = 1;
        return ;
    }
    if (ext_status == 1)
    {
        scancode = (0xe0<<8 | scancode);
        ext_status = 0;
    }

    Bool shift = 0;
    Bool breakcode = scancode&0x80;
    if (breakcode)
    {
        uint_8 makecode = scancode & 0xff7f;
        code_swistat(makecode,0);
        updatestat();
        return ;
    }
    else if ((scancode>0 && scancode<0x3b) || \
             (scancode == ctrl_r_make)     || \
             (scancode == alt_r_make)) 
    {
        if (IS_DOUBLECHAR(scancode)) {
            shift = shift_status;
        } else {
            shift = shift_status ^ caps_lock_status;
        }

        uint_8 index = scancode;
        char cur_char = keymap[index][shift];
        if (ctrl_status && (cur_char=='l' || cur_char=='u')) {
            cur_char -= 'a';
        }
        if (cur_char) {
            if (!ioq_full(&kbd_buf)){
                ioq_putchar(&kbd_buf,cur_char);
            }
            return ;
        }

        code_swistat(scancode,1);
        updatestat();
        if (scancode == caps_make)
            caps_lock_status = !caps_lock_status;
    }
    else
    {
        //put_str("unknown char\n");
    }
}

void keyboard_init(void)
{
    put_str("keyboard init start\n");
    ioq_init(&kbd_buf);
    register_handler(0x21,intr_keyboard_handler);
    put_str("keyboard init done\n");
}

