%include "const.S"

extern current
extern syscall_handler_table

global my_syscall

[SECTION .text]
my_syscall:
    SAVE_STATE

    ; handler arguments
    push dword [current]
    push ecx
    push ebx

    call [syscall_handler_table + 4*eax] ; sizeof(void *) == 4
    add esp, 4*3 ; free arguments

    mov esi, [current]
    mov [esi+OFFSET_PROC_STACK0-6*4], eax ; change eax(from pusha) for return value

    RESTORE_STATE
    iret
