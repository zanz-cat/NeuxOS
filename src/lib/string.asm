%include "include/common.inc"

global memcpy
global memset

[SECTION .text]
; void* memcpy(void *dst, void *src, u32 size);
memcpy:
    push ebp
    mov ebp, esp

    push edi
    push esi
    push ecx
    push es

    mov edi, arg(0)    ; Destination
    mov esi, arg(1)   ; Source
    mov ecx, arg(2)   ; Size
    rep movsb

    pop es
    pop ecx
    pop esi
    pop edi
    
    pop ebp
    ret

; void *memset(void *s, int ch, u32 size);
memset:
    push ebp
    mov ebp, esp
    
    push eax
    push edi
    push ecx

    xor eax, eax
    mov edi, arg(0)
    mov ecx, arg(1)
    rep stosb

    pop ecx
    pop edi
    pop eax

    pop ebp
    ret