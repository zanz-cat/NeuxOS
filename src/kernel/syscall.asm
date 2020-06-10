SYSCALL_GET_TICKS       equ     0

global syscall

[SECTION .data]
syscall_handler_table:
dd get_ticks

[SECTION .text]
syscall:
    xchg bx, bx
    call [syscall_handler_table + 4*eax]
    ret

get_ticks:
    xchg bx, bx
    ret