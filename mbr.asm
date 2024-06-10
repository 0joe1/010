%include "boot.inc"
;---------------------------------------------------------
SECTION mbr vstart=0x7c00
        mov ax,0xb800
        mov ds,ax
        mov gs,ax
        mov ax,cs
        mov es,ax
        mov ss,ax
        mov sp,0x7c00

;---------------------------------------------------------
;INT 0x10 功能号:0x06 功能描述:上卷窗口
;---------------------------------------------------------
;input
;ah=功能号,al=上卷行数(若为0,表示全部),bh=上卷行属性
;(cl,ch)= 窗口左上角(X,Y)
;(dl,dh)= 窗口右下角(X,Y) 其中X为横坐标,Y为纵坐标
;no return
        mov cx,0x00
        mov dx,0x184f
        mov ax,0x600
        mov bx,0x700

        int 0x10

;---------------------------------------------------------
        mov byte[0x00],'M'
        mov byte[0x01],0x61 ;棕底绿字

        mov byte[0x02],'B'
        mov byte[0x03],0x61

        mov byte[0x04],'R'
        mov byte[0x05],0x61


        mov eax,LOADER_START_SECTION
        mov bx,LOADER_BASE_ADDR
        mov ecx,3
        call read_disk
        jmp LOADER_BASE_ADDR+0x300

;---------------------------------------------------------
;eax=起始lba地址
;bx=读入内存地址
;cx=读入的扇区数
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
        mov [cs:bx],ax
        add bx,2
        loop .write

        ret

;---------------------------------------------------------
times 510-($-$$) db 0
                 db 0x55,0xaa
