global memcpy

[SECTION .text]
; void* memcpy(void *dst, void *src, u32 size);
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
    rep movsb

    pop es
    pop ecx
    pop esi
    pop edi
    
    pop ebp
    ret

; void *memset(void *s, int ch, u32 size);
memset:
    ; TODO
    ret