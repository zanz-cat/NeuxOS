%include "const.S"

extern current
extern do_syscall

global irq_ex_syscall

[SECTION .text]
irq_ex_syscall:
    SAVE_STATE

    ; TODO arpl
    push eax
    call do_syscall
    add esp, 4

    mov [esp+4*11], eax ; return value

    RESTORE_STATE
    iret
