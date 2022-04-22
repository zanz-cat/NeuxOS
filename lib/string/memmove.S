%define arg(i) [ebp+4*(i+2)]

global memmove

[SECTION .text]
; void* memmove(void *dest, void *src, uint32_t size);
memmove:
    push ebp
    mov ebp, esp

    push edi
    push esi
    push ecx

    mov edi, arg(0)   ; destination
    mov esi, arg(1)   ; source
    mov ecx, arg(2)   ; size
    cmp esi, edi ; src < dest && src + n > dest => overlap
    jnb .a1
    add esi, ecx
    cmp edi, esi
    jnb .a1
    add edi, ecx
    std
    rep movsb
    cld
    jmp .fin
.a1:
    mov esi, arg(1)
    rep movsb
.fin:
    pop ecx
    pop esi
    pop edi

    leave
    ret