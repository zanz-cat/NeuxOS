%include "include/const.inc"

%macro SAVE_STATE 0
    pusha
    push ds
    push es
    push fs
    push gs
%endmacro

%macro RESTORE_STATE 0
    pop gs
    pop fs
    pop es
    pop ds
    popa
%endmacro

%macro hwint 2
    ; save state
    SAVE_STATE
    mov ax, SELECTOR_KERNEL_DS
    mov ds, ax
    mov es, ax

    ; close current interrupt
    %if %1 = 0
        in al, INT_M_CTLMASK
        or al, (1 << %2)
        out INT_M_CTLMASK, al
    %else
        in al, INT_S_CTLMASK
        or al, (1 << %2)
        out INT_S_CTLMASK, al
    %endif
    ; enable interrupt
    sti
    ; save proc stack info if is kernel proc
    mov eax, [current]
    cmp word [eax+OFFSET_PROC_TYPE], 0
    jne .skip
    mov [eax+OFFSET_PROC_ESP], esp
    mov [eax+OFFSET_PROC_SS], ss
.skip:
    ; switch to system stack
    mov ax, SELECTOR_KERNEL_DS
    mov ss, ax
    mov esp, sys_stacktop
    ; call handler
    call [hw_irq_handler_table + 4*%1]
    ; switch to kernel stack of proc
    mov eax, [current]
    lldt word [eax+OFFSET_PROC_LDT_SEL]
    cmp word [eax+OFFSET_PROC_TYPE], 0
    jne .user_proc
    mov ss, [eax+OFFSET_PROC_SS]
    mov esp, [eax+OFFSET_PROC_ESP]
    jmp .fini
.user_proc:
    lea ebx, [eax+OFFSET_PROC_PID]  ; kernel stack top of user proc
    mov [tss+OFFSET_TSS_ESP0], ebx
    mov esp, eax
    mov ax, SELECTOR_KERNEL_DS
    mov ss, ax
.fini:
    ; open current interrupt
    %if %1 = 0
        in al, INT_M_CTLMASK
        and al, ~(1 << %2)
        out INT_M_CTLMASK, al
    %else
        in al, INT_S_CTLMASK
        and al, ~(1 << %2)
        out INT_S_CTLMASK, al
    %endif
    ; send eoi
    mov al, INT_EOI
    out INT_M_CTL, al
    ; restore state
    RESTORE_STATE
    iret
%endmacro

extern hw_irq_handler_table
extern current
extern sys_stacktop
extern tss
extern exception_handler

global divide_error
global single_step_exception
global nmi
global breakpoint_exception
global overflow
global bounds_check
global inval_opcode
global copr_not_available
global double_fault
global copr_seg_overrun
global inval_tss
global segment_not_present
global stack_exception
global general_protection
global page_fault
global copr_error

global hwint00
global hwint01
global hwint03
global hwint04
global hwint05
global hwint06
global hwint07
global hwint08
global hwint12
global hwint13
global hwint14

[SECTION .text]
divide_error:
    push 0ffffffffh
    push 0
    jmp exception
single_step_exception:
    push 0ffffffffh
    push 1
    jmp exception
nmi:
    push 0ffffffffh
    push 2
    jmp exception
breakpoint_exception:
    push 0ffffffffh
    push 3
    jmp exception
overflow:
    push 0ffffffffh
    push 4
    jmp exception
bounds_check:
    push 0ffffffffh
    push 5
    jmp exception
inval_opcode:
    push 0ffffffffh
    push 6
    jmp exception
copr_not_available:
    push 0ffffffffh
    push 7
    jmp exception
double_fault:
    push 0ffffffffh
    push 8
    jmp exception
copr_seg_overrun:
    push 0ffffffffh
    push 9
    jmp exception
inval_tss:
    push 10
    jmp exception
segment_not_present:
    push 11
    jmp exception
stack_exception:
    push 12
    jmp exception
general_protection:
    push 13
    jmp exception
page_fault:
    push 14
    jmp exception
copr_error:
    push 0ffffffffh
    push 16
    jmp exception
exception:
    call exception_handler
    add esp, 4*2
    iret

hwint00:
    hwint 0, 0
    
hwint01:
    hwint 0, 1

hwint03:
    hwint 0, 3
    
hwint04:
    hwint 0, 4
    
hwint05:
    hwint 0, 5
    
hwint06:
    hwint 0, 6
    
hwint07:
    hwint 0, 7
    
hwint08:
    hwint 1, 0
    
hwint12:
    hwint 1, 4
    
hwint13:
    hwint 1, 5
    
hwint14:
    hwint 1, 6

