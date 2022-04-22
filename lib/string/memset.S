%define arg(i) [ebp+4*(i+2)]

global memset

[SECTION .text]
; void *memset(void *s, int ch, size_t size);
; arg(i) [ebp+4*(i+2)]
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

    leave
    ret