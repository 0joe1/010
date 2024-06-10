%include "boot.inc"
;----------------------------------------------------------
SECTION loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR
;下面数据大小凑成0x300
        GDT_NULL dd 0x00
                 dd 0x00

        CODE_SEG  dd  0x0000ffff
                  dd  DESC_CODE_HIGH4

        DATA_SEG  dd  0x0000ffff
                  dd  DESC_DATA_HIGH4

        VIDIO_SEG dd  0x80000007
                  dd  DESC_VIDEO_HIGH4

        GDT_SIZE  equ  $-GDT_NULL
        GDT_LIMIT equ  GDT_SIZE-1

        times 60  dq  0          ;预留空位

        SELECTOR_CODE  equ (0x01<<3) + TI_GDT + DPL_0
        SELECTOR_DATA  equ (0x02<<3) + TI_GDT + DPL_0
        SELECTOR_VIDEO equ (0x03<<3) + TI_GDT + DPL_0

        ;0x200
        total_mem_bytes  dd  0

        gdt_ptr   dw  GDT_LIMIT
                  dd  GDT_NULL


        ards_buf  times 244 db 0
        ards_nr   dw 0           ;dw两个字节
    .e820
        mov ax,cs
        mov ds,ax
        xor ebx,ebx
        mov edx,0x534d4150
        mov di,ards_buf
    .mem_get_loop
        mov ecx,20
        mov eax,0xe820
        int 0x15
        jc .try_e801
        inc word[ards_nr]
        add di,cx
        cmp ebx,0
        jnz .mem_get_loop

        mov cx,[ards_nr]
        mov di,ards_buf
        xor edx,edx
    .select_big
        mov eax,[es:di]
        add eax,[es:di+8]
        cmp eax,edx
        jle .next_ards
        mov edx,eax
    .next_ards
        add di,20
        loop .select_big
        jmp .mem_get_ok

    .try_e801
        mov eax,0xe801
        int 0x15
        jc .try_88

        ;先算出低15MB的内存
        ;ax和cx中是以KB为单位的内存数量，将其转换为以byte为单位
        mov cx,0x400
        mul cx
        shl edx,16
        and eax,0x0000ffff
        or edx,eax
        add edx,0x100000  ;ax只是15MB,所以要加1MB内存空洞
        mov esi,edx

        xor eax,eax
        mov ax,bx
        mov ecx,0x1000
        mul ecx           ;32位乘法，默认的被乘数是eax,积为edx:eax
        add esi,eax
        mov edx,esi
        jmp .mem_get_ok

    .try_88
        mov ah,0x88
        int 0x15
        and eax,0x0000ffff

        mov cx,0x400
        mul cx
        shl edx,16
        or edx,eax
        add edx,0x100000   ;0x88子功能只会返回1MB以上的内存

    .mem_get_ok
        mov [total_mem_bytes],edx
;------------------------------------------------------------
        ;A20
        in al,0x92
        or al,0000_0010b
        out 0x92,al

        ;加载gdtr寄存器
        lgdt [cs:gdt_ptr]

        ;进入保护模式
        mov eax,cr0
        or eax,1
        mov cr0,eax

        ;清空流水线并更新段描述符缓冲寄存器
        jmp SELECTOR_CODE:protect_mode
;------------------------------------------------------------
    [bits 32]
    protect_mode:
        mov ax,SELECTOR_DATA
        mov ds,ax
        mov ss,ax
        mov es,ax
        mov esp,LOADER_STACK_TOP
        mov ax,SELECTOR_VIDEO
        mov gs,ax
;------------------read kernel from disk---------------------
        mov eax,KERNEL_START_SECTION
        mov ebx,KERNEL_BASE_ADDR
        mov ecx,200

        call read_disk

;-------------------set_up_pages-----------------------------
        call setup_page

        mov ebx,[gdt_ptr+2]
        add dword[ebx+24+4],0xc0000000
        add esp,0xc0000000

        mov eax,cr3
        or eax,0x100000
        mov cr3,eax

        mov eax,cr0
        or eax,0x80000000
        mov cr0,eax

        add dword[gdt_ptr+2],0xc0000000
        lgdt [gdt_ptr]

        jmp SELECTOR_CODE:enter_kernel
;------------------------------------------------------------
    enter_kernel:
        mov esp,0xc009f000
        call kernel_init

        jmp KERNEL_ENTRY

;------------------------------------------------------------
    setup_page:
        mov ecx,4096
        mov eax,0
        xor esi,esi
    .clear_dir
        mov [PAGE_DIR_BASE+esi],eax
        inc esi
        loop .clear_dir

        mov eax,PAGE_DIR_BASE
        or eax,PAGE_P | PAGE_RW_W | PAGE_US_U

        mov [PAGE_DIR_BASE+4092],eax

        add eax,0x1000 ;加上4KB，到了第一个页表
        mov [PAGE_DIR_BASE],eax
        mov [PAGE_DIR_BASE+0xc00],eax

        ;解决”最初1MB“映射问题
        mov ecx,256
        mov ebx,PAGE_DIR_BASE+0x1000
        xor esi,esi
        mov edx,0
        or edx,PAGE_P | PAGE_RW_W | PAGE_US_U
    .first_1mb
        mov [ebx+esi*4],edx
        inc esi
        add edx,0x1000
        loop .first_1mb

        mov ecx,254
        mov esi,PAGE_DIR_BASE+0xc00
    .fill_ker_pages
        add eax,0x1000
        add esi,4
        mov [esi],eax
        loop .fill_ker_pages

        ret
;----------------------kernel_loader-------------------------
    kernel_init:
        xor edx,edx
        xor ecx,ecx

        mov dx,[KERNEL_BASE_ADDR+42]
        mov ebx,[KERNEL_BASE_ADDR+28]
        add ebx,KERNEL_BASE_ADDR
        mov cx,[KERNEL_BASE_ADDR+44]

    load_kernel:
        mov eax,[ebx]
        cmp eax,PT_NULL
        je .next_Phdr

        push dword[ebx+16]
        push dword[ebx+8]
        mov eax,[ebx+4]
        add eax,KERNEL_BASE_ADDR
        push eax
        call mem_copy
        add esp,12

    .next_Phdr
        add ebx,edx
        loop load_kernel

        ret

;-----------------------memory_copy-------------------------
;(src,dst,size) -> void
    mem_copy:
        push ebp
        mov ebp,esp
        push ecx

        cld
        mov esi,[ebp+8]
        mov edi,[ebp+12]
        mov ecx,[ebp+16]
        rep movsb

        pop ecx
        pop ebp

        ret
;-----------------------read_disk----------------------------
;eax=起始lba地址
;ebx=读入内存地址
;ecx=读入的扇区数
read_disk:
        mov edi,eax

        mov dx,0x1f2
        mov al,cl
        out dx,al

        ;0x1f3
        mov eax,edi
        inc dx
        out dx,al

        ;0x1f4
        inc dx
        shr eax,8
        out dx,al

        ;0x1f5
        inc dx
        shr eax,8
        out dx,al

        inc dx
        shr eax,4
        and al,0x0f
        or al,0xe0   ;0代表主盘，1代表从盘
        out dx,al

        mov dx,0x1f7
        mov al,0x20
        out dx,al
    .wait
        nop          ;相当于sleep一下
        in al,dx     ;这里8位，用al而非ax
        and al,0x88
        cmp al,0x08
        jnz .wait

        mov ax,256
        mul cx
        mov cx,ax
        mov dx,0x1f0
    .write
        in ax,dx
        mov [ebx],ax    ;小样，加个e让爷好找一个晚上
        add ebx,2
        loop .write

        ret

