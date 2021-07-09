%include "common.S"

%macro hwint 1
    ; save state
    SAVE_STATE
    mov ax, SELECTOR_KERNEL_DS
    mov ds, ax
    mov es, ax

    ; close current interrupt
    %if %1 < 8
        in al, INT_M_CTLMASK
        or al, (1 << %1)
        out INT_M_CTLMASK, al
    %else
        in al, INT_S_CTLMASK
        or al, (1 << (%1 % 8))
        out INT_S_CTLMASK, al
    %endif

    ; save kernel stack info of interrupted routine
    lea ebx, [hwint_stacks + HWINT_STACK_SIZE*%1]
    mov eax, [current]
    cmp eax, 0
    je  .non_proc
    ; save kernel stack info to proc
    mov [eax + OFFSET_PROC_ESP0], esp
    mov [eax + OFFSET_PROC_SS0], ss
    mov dword [current], 0
    ; save dummy stack info to interrupt stack
    sub ebx, 2
    mov word [ebx], 0
    sub ebx, 4
    mov dword [ebx], 0
    jmp .kern_stack_saved
.non_proc:
    ; save stack info to nested interrupt stack
    sub ebx, 2
    mov [ebx], ss
    sub ebx, 4
    mov [ebx], esp
.kern_stack_saved:

    ; switch to interrupt stack
    mov ax, SELECTOR_KERNEL_DS
    mov ss, ax
    mov esp, ebx

    ; enable interrupt
    sti

    ; call handler
    call [irq_handler_table + 4*%1]

    ; disable interrupt
    cli

    cmp dword [esp], 0
    je  .sched_proc
    ; restore outer kernel stack
    mov ebx, esp
    mov esp, [ebx]
    add ebx, 4
    mov ss, [ebx]
    jmp .next_selected
.sched_proc:
    %if %1 != 0
        call proc_sched
    %endif
    mov eax, [current]
    ; load proc ldt
    lldt word [eax+OFFSET_PROC_LDT_SEL]
    ; setup tr.esp0
    lea ebx, [eax + OFFSET_PROC_STACK0]
    mov [tss+OFFSET_TSS_ESP0], ebx
    ; restore proc kernel stack
    mov ss, [eax+OFFSET_PROC_SS0]
    mov esp, [eax+OFFSET_PROC_ESP0]
.next_selected:

    ; open current interrupt
    %if %1 < 8
        in al, INT_M_CTLMASK
        and al, ~(1 << %1)
        out INT_M_CTLMASK, al
    %else
        in al, INT_S_CTLMASK
        and al, ~(1 << (%1 % 8))
        out INT_S_CTLMASK, al
    %endif

    ; send eoi
    mov al, INT_EOI
    out INT_M_CTL, al
    
    ; restore state
    RESTORE_STATE
    iret
%endmacro

extern tss
extern current
extern irq_handler_table
extern exception_handler
extern proc_sched

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

[SECTION .bss]
resb    HWINT_STACK_SIZE * 16
hwint_stacks:

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

