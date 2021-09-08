%include "const.S"

%macro hwint 1
    ; save state
    SAVE_STATE

    ; call handler
    call [irq_handlers + 4*%1]

    ; send eoi
    mov al, INT_EOI
    out INT_M_CTL, al

%if %1 = 0
    mov eax, [current]
    str bx
    cmp bx, [eax+OFFSET_TASK_TSS_SEL]
    je .continue
    jmp far [eax]
%endif

.continue:
    ; restore state
    RESTORE_STATE
    iret
%endmacro

extern current
extern irq_handlers
extern irq_ex_handlers

global irq_ex_divide_error
global irq_ex_single_step
global irq_ex_nmi
global irq_ex_breakpoint
global irq_ex_overflow
global irq_ex_bounds_check
global irq_ex_inval_opcode
global irq_ex_copr_not_available
global irq_ex_double_fault
global irq_ex_copr_seg_overrun
global irq_ex_inval_tss
global irq_ex_segment_not_present
global irq_ex_stack_exception
global irq_ex_general_protection
global irq_ex_page_fault
global irq_ex_copr_error

global irq_hwint00
global irq_hwint01
global irq_hwint03
global irq_hwint04
global irq_hwint05
global irq_hwint06
global irq_hwint07
global irq_hwint08
global irq_hwint12
global irq_hwint13
global irq_hwint14

%macro call_ex_handler 1
    push %1 ; vector_no
    call [irq_ex_handlers + 4 * %1]
    add esp, 4*2
%endmacro

[SECTION .text]
irq_ex_divide_error:
    push 0ffffffffh ; err_code
    call_ex_handler 0
    iret
irq_ex_single_step:
    push 0ffffffffh
    call_ex_handler 1
    iret
irq_ex_nmi:
    push 0ffffffffh
    call_ex_handler 2
    iret
irq_ex_breakpoint:
    push 0ffffffffh
    call_ex_handler 3
    iret
irq_ex_overflow:
    push 0ffffffffh
    call_ex_handler 4
    iret
irq_ex_bounds_check:
    push 0ffffffffh
    call_ex_handler 5
    iret
irq_ex_inval_opcode:
    push 0ffffffffh
    call_ex_handler 6
    iret
irq_ex_copr_not_available:
    push 0ffffffffh
    call_ex_handler 7
    iret
irq_ex_double_fault:
    push 0ffffffffh
    call_ex_handler 8
    iret
irq_ex_copr_seg_overrun:
    push 0ffffffffh
    call_ex_handler 9
    iret
irq_ex_inval_tss:
    call_ex_handler 10
    iret
irq_ex_segment_not_present:
    call_ex_handler 11
    iret
irq_ex_stack_exception:
    call_ex_handler 12
    iret
irq_ex_general_protection:
    call_ex_handler 13
    iret
irq_ex_page_fault:
    call_ex_handler 14
    iret
irq_ex_copr_error:
    push 0ffffffffh
    call_ex_handler 16
    iret

irq_hwint00:
    hwint 0

irq_hwint01:
    hwint 1

irq_hwint03:
    hwint 3

irq_hwint04:
    hwint 4

irq_hwint05:
    hwint 5

irq_hwint06:
    hwint 6

irq_hwint07:
    hwint 7

irq_hwint08:
    hwint 8

irq_hwint12:
    hwint 12

irq_hwint13:
    hwint 13

irq_hwint14:
    hwint 14