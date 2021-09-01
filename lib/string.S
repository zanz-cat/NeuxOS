%define arg(i) [ebp+4*(i+2)]

global memcpy
global memmove
global memset

[SECTION .text]
; void* memcpy(void *dst, void *src, uint32_t size);
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

    pop ebp
    ret

; void* memmove(void *dst, void *src, uint32_t size);
memmove:
    push ebp
    mov ebp, esp

    push edi
    push esi
    push ecx

    mov edi, arg(0)   ; destination
    mov esi, arg(1)   ; source
    mov ecx, arg(2)   ; size
    cmp esi, edi ; src < dst && src + n > dst => overlap
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

    pop ebp
    ret

; void *memset(void *s, int ch, uint32_t size);
memset:
    push ebp
    mov ebp, esp

    push eax
    push edi
    push ecx

    mov edi, arg(0)
    mov eax, arg(1)
    mov ecx, arg(2)
    rep stosb

    pop ecx
    pop edi
    pop eax

    pop ebp
    ret