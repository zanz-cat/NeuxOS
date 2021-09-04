%include "const.S"

%macro hwint 1
    ; save state
    SAVE_STATE

    ; call handler
    call [irq_handler_table + 4*%1]

    ; send eoi
    mov al, INT_EOI
    out INT_M_CTL, al

%if %1 = 0
    mov eax, [current]
    str bx
    cmp bx, [eax+OFFSET_TASK_TSS_SEL]
    je .continue
    jmp far [eax+OFFSET_TASK_TSS_SEL-4]
%endif

.continue:
    ; restore state
    RESTORE_STATE
    iret
%endmacro

extern current
extern irq_handler_table
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
    push 0ffffffffh ; err_code
    push 0  ; vector_no
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
    hwint 0

hwint01:
    hwint 1

hwint03:
    hwint 3

hwint04:
    hwint 4

hwint05:
    hwint 5

hwint06:
    hwint 6

hwint07:
    hwint 7

hwint08:
    hwint 8

hwint12:
    hwint 12

hwint13:
    hwint 13

hwint14:
    hwint 14