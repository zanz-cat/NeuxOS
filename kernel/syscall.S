%include "const.S"

extern current
extern syscall_handler_table

global my_syscall

[SECTION .text]
my_syscall:
    SAVE_STATE
    ; handler's arguments
    push dword [current]
    push ecx
    push ebx

    ; TODO arpl
    call [syscall_handler_table + 4*eax] ; sizeof(void *) == 4
    add esp, 4*3 ; free arguments

    mov [esp+4*11], eax ; return value

    RESTORE_STATE
    iret
