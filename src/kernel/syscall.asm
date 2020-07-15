%include "include/const.inc"
%include "include/common.inc"

extern current
extern syscall_handler_table

global syscall

[SECTION .text]
syscall:
    SAVE_STATE

    call [syscall_handler_table + 4*eax]

    mov esi, [current]
    mov [esi+OFFSET_PROC_STACK0-6*4], eax

    RESTORE_STATE
    iret
