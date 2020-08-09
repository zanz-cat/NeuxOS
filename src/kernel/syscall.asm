%include "include/const.inc"
%include "include/common.inc"

extern current
extern syscall_handler_table

global syscall

[SECTION .text]
syscall:
    SAVE_STATE

    push dword [current]
    push ecx
    push ebx
    call [syscall_handler_table + 4*eax]
    add esp, 4*3
    
    mov esi, [current]
    mov [esi+OFFSET_PROC_STACK0-6*4], eax

    RESTORE_STATE
    iret
