org 0x100
xchg bx, bx
jmp LABEL_START

LOADING_MESSAGE      db  'LOADING...'
LOADING_MESSAGE_LEN  equ $ - LOADING_MESSAGE

times 1024 db 0

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov es, ax

    call DispEnter

    mov bp, LOADING_MESSAGE
    mov cx, LOADING_MESSAGE_LEN
    call DispStr

    jmp $

%include "common.inc"
