%include "const.S"

extern current
extern syscall_handler_table

global irq_ex_syscall

[SECTION .text]
irq_ex_syscall:
    SAVE_STATE

    ; handler's arguments
    push edx ; arg2
    push ecx ; arg1
    push ebx ; arg0

    ; use 'arpl' to ensure safety???
    call [syscall_handler_table + 4*eax] ; sizeof(void *) == 4
    add esp, 4*3 ; free arguments

    mov [esp+4*11], eax ; modify eax in memory saved by pusha as return value

    RESTORE_STATE
    iret