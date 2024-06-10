    [bits 32]
    %define ERR_CODE nop
    %define NONE push 0

    section .data
    int_msg db 'interupt occur',0x0a,0

    extern put_str  ;利用extern引入外部符号
    extern idt_table 
    extern syscall_table

    %macro VECTOR 2

    section .text
    int%1entry:
        %2
        push ds
        push es
        push fs
        push gs
        pushad

        ;若这个不放在前面的话，后面switch没ret到这里导致中断不发生
        mov al,0x20
        out 0x20,al
        out 0xa0,al

        push %1
        call [idt_table+%1*4]

        jmp int_exit

    section .data
        dd int%1entry

    %endmacro

section .text
global syscall_handler
syscall_handler:
    push 0
    push ds
    push es
    push fs
    push gs
    pushad
    push 0x80 ;占位而已，0x80实际没意义，与前面压中断号保持队形罢了

    push edx
    push ecx
    push ebx
    call [syscall_table + eax*4]
    add esp,12

    mov [esp+0x20],eax
    jmp int_exit

section .text
global int_exit
int_exit:
    add esp,4  ;跳过压入的中断号，为了配合intr_stack,放在这里了
    popad
    pop gs
    pop fs
    pop es
    pop ds

    add esp,4
    iret

section .data         ;idt_desc_addr需要是.data才能与后面的data合并到一个segment
global idt_desc_addr
idt_desc_addr:
    VECTOR 0x00,NONE
    VECTOR 0x01,NONE
    VECTOR 0x02,NONE
    VECTOR 0x03,NONE
    VECTOR 0x04,NONE
    VECTOR 0x05,NONE
    VECTOR 0x06,NONE
    VECTOR 0x07,NONE
    VECTOR 0x08,ERR_CODE
    VECTOR 0x09,NONE
    VECTOR 0x0a,NONE
    VECTOR 0x0b,NONE
    VECTOR 0x0c,NONE
    VECTOR 0x0d,NONE
    VECTOR 0x0e,NONE
    VECTOR 0x0f,NONE
    VECTOR 0x10,NONE
    VECTOR 0x11,ERR_CODE
    VECTOR 0x12,NONE
    VECTOR 0x13,NONE
    VECTOR 0x14,NONE
    VECTOR 0x15,NONE
    VECTOR 0x16,NONE
    VECTOR 0x17,NONE
    VECTOR 0x18,NONE
    VECTOR 0x19,NONE
    VECTOR 0x1a,NONE
    VECTOR 0x1b,NONE
    VECTOR 0x1c,NONE
    VECTOR 0x1d,NONE
    VECTOR 0x1e,NONE
    VECTOR 0x1f,NONE
    VECTOR 0x20,NONE
    VECTOR 0x21,NONE
    VECTOR 0x22,NONE
    VECTOR 0x23,NONE
    VECTOR 0x24,NONE
    VECTOR 0x25,NONE
    VECTOR 0x26,NONE
    VECTOR 0x27,NONE
    VECTOR 0x28,NONE
    VECTOR 0x29,NONE
    VECTOR 0x2a,NONE
    VECTOR 0x2b,NONE
    VECTOR 0x2c,NONE
    VECTOR 0x2d,NONE
    VECTOR 0x2e,NONE
    VECTOR 0x2f,NONE
