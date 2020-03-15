jmp LABEL_START

%include "common.inc"


LOADING_MESSAGE      db  'LOADING...'
LOADING_MESSAGE_LEN  equ $ - LOADING_MESSAGE

times 50000 db 0

LABEL_START:
call DispEnter

mov ax, 09010h
mov es, ax
mov bp, LOADING_MESSAGE
mov cx, LOADING_MESSAGE_LEN
call DispStr

jmp $
