org 0x100
jmp LABEL_START

%include "loader.inc"

LOADING_MESSAGE      db  'Loading...'
LOADING_MESSAGE_LEN  equ $ - LOADING_MESSAGE

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

