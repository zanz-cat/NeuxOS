%define arg(i) [ebp+4*(i+2)]

global memcpy
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