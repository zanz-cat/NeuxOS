global memcpy

[SECTION .text]
memcpy:
    xchg bx, bx
    mov ax, 100
    ret
