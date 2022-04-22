%define arg(i) [ebp+4*(i+2)]

global memcpy

[SECTION .text]
; void* memcpy(void *dest, void *src, uint32_t size);
memcpy:
    push ebp
    mov ebp, esp

    push edi
    push esi
    push ecx

    mov edi, arg(0)   ; destination
    mov esi, arg(1)   ; source
    mov ecx, arg(2)   ; size
    rep movsb

    pop ecx
    pop esi
    pop edi

    leave
    ret