global memcpy

[SECTION .text]
memcpy:
    push ebp
    mov ebp, esp

    push edi
    push esi
    push ecx
    push es

    mov edi, [ebp+8]    ; Destination
    mov esi, [ebp+12]   ; Source
    mov ecx, [ebp+16]   ; Size
    mov ax, ds
    mov es, ax
    rep movsb

    pop es
    pop ecx
    pop esi
    pop edi
    
    pop ebp
    ret
