DPL_0     equ  00b
TI_GDT    equ  000b
SELECTOR_VIDEO equ (0x03<<3) + TI_GDT + DPL_0

section .data
    int_buffer dq 0

    [bits 32]
section .text
global put_char
global put_str
global put_int
global set_cursor
global cls_screen

    put_char:
        pushad
        mov ax,SELECTOR_VIDEO
        mov gs,ax

        mov dx,0x3d4
        mov al,0xe
        out dx,al
        inc dx
        in al,dx
        mov ah,al

        mov dx,0x3d4
        mov al,0xf
        out dx,al
        inc dx
        in al,dx
        mov bx,ax

        mov ecx,[esp+36]
        cmp cl,0x8
        jz .backspace
        cmp cl,0xd
        jz .LF
        cmp cl,0xa
        jz .LF

    .putc
        shl bx,1
        mov [gs:bx],cl
        shr bx,1
        inc bx
        jmp .roll_screen

    .backspace
        dec bx
        shl bx,1
        mov byte[gs:bx],0x20
        shr bx,1
        jmp .roll_screen

    .LF
        add bx,80
    .CR
        mov ax,bx
        mov bl,80
        div bl
        mul bl
        mov bx,ax

    .roll_screen
        cmp bx,2000
        jl .set_cursor
        mov ax,SELECTOR_VIDEO
        cld
        mov esi,0xc00b80a0
        mov edi,0xc00b8000   ;从0开始
        mov ecx,960 ;每个字符占两个字节（还有属性位）
        rep movsd

        mov ecx,80
        mov bx,3840
    .clear
        mov word[gs:bx],0x0720
        add bx,2
        loop .clear
        mov bx,1920

    .set_cursor
        mov dx,0x3d4
        mov al,0xe
        out dx,al
        inc dx
        mov al,bh
        out dx,al

        mov dx,0x3d4
        mov al,0xf
        out dx,al
        inc dx
        mov al,bl
        out dx,al

        popad
        ret

;------------------------------------------------------------
    put_str:
        push ebx
        push ecx

        xor ecx,ecx
        mov ebx,[esp+12]
    .goon
        mov cl,[ebx]
        cmp cl,0
        jz .str_over
        push ecx
        call put_char
        add esp,4
        inc ebx
        jmp .goon

    .str_over
        pop ecx
        pop ebx
        ret
;------------------------------------------------------------
    put_int:
        pushad

        mov eax,[esp+36]
        mov edx,eax
        mov ecx,8
        mov edi,7
    .put_in_buffer
        and edx,0xf
        cmp dl,9
        jg .a2f
        add dl,'0'
        jmp .fill_buffer
    .a2f
        sub dl,10
        add dl,'A'
    .fill_buffer
        mov [int_buffer+edi],dl
        dec edi
        shr eax,4
        mov edx,eax
        loop .put_in_buffer

        inc edi
    .skip_zeros
        cmp edi,8
        jz .all_zero
        mov cl,[int_buffer+edi]
        cmp cl,'0'
        jnz .output_int
        inc edi
        jmp .skip_zeros

    .all_zero
        mov cl,'0'
    .output_int
        push ecx
        call put_char
        add esp,4
        inc edi
        cmp edi,8
        jge .out
        mov cl,[int_buffer+edi]
        jmp .output_int

    .out
        popad
        ret

;------------------------------------------------------------
set_cursor
    push ebp
    push ebx
    push edi
    push esi

    mov bx,[esp+0x14]
    mov dx,0x3d4
    mov al,0xe
    out dx,al
    inc dx
    mov al,bh
    out dx,al

    mov dx,0x3d4
    mov al,0xf
    out dx,al
    inc dx
    mov al,bl
    out dx,al

    pop esi
    pop edi
    pop ebx
    pop ebp
    ret

;------------------------------------------------------------
cls_screen
    pushad
    mov ax,SELECTOR_VIDEO
    mov gs,ax

    mov ebx,0
    mov ecx,80*25

.clear
    mov word[gs:ebx],0x0720
    add ebx,2
    loop .clear
    mov ebx,0

    mov dx,0x3d4
    mov al,0xe
    out dx,al
    inc dx
    mov al,bh
    out dx,al

    mov dx,0x3d4
    mov al,0xf
    out dx,al
    inc dx
    mov al,bl
    out dx,al

    popad
    ret
