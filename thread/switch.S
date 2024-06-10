global switch_to
    switch_to:
        push ebp
        push ebx
        push edi
        push esi

        mov eax,[esp+0x14]
        mov [eax],esp
        ;----------- next thread -----------
        mov eax,[esp+0x18]
        mov esp,[eax]

        pop esi
        pop edi
        pop ebx
        pop ebp
        ret

        
