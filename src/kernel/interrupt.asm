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

; save stack info to system stack and switch to system stack
%macro SWTICH_TO_SYSSTACK 0
    mov [sys_stacktop - 4], ss
    mov [sys_stacktop - 8], esp
    mov ax, SELECTOR_KERNEL_DS
    mov ss, ax
    lea esp, [sys_stacktop - 8]    
%endmacro

; switch back to kernel stack of proc
%macro SWTICH_BACK_TO_USERSTACK 0
    mov esp, [sys_stacktop - 8]
    mov ss, [sys_stacktop - 4]
%endmacro

extern exception_handler
extern send_eoi

extern clock_handler
extern keyboard_int
extern serial2_int
extern serial1_int
extern lpt2_int
extern floppy_int
extern lpt1_int
extern real_clock_int
extern mouse_int
extern copr_int
extern harddisk_int
extern clock_int_stacktop

extern current
extern sys_stacktop
extern tss

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

[SECTION .data]
clock_reentry   db  0

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
    SAVE_STATE
    mov ax, SELECTOR_KERNEL_DS
    mov ds, ax
    mov es, ax

    ; cmp byte [clock_reentry], 0
    ; jne .reentry_skip
    ; inc byte [clock_reentry]
    ; sti
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
    ; schedule
    call clock_handler
    call send_eoi
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
;     dec byte [clock_reentry]
; .reentry_skip:
    RESTORE_STATE
    iret
    
hwint01:
    SAVE_STATE
    mov ax, SELECTOR_KERNEL_DS
    mov ds, ax
    mov es, ax
    SWTICH_TO_SYSSTACK
    
    ; handle
    call keyboard_int
    call send_eoi

    SWTICH_BACK_TO_USERSTACK
    RESTORE_STATE
    iret
hwint03:
    call serial2_int
    call send_eoi
    iret
hwint04:
    call serial1_int
    call send_eoi
    iret
hwint05:
    call lpt2_int
    call send_eoi
    iret
hwint06:
    call floppy_int
    call send_eoi
    iret
hwint07:
    call lpt1_int
    call send_eoi
    iret
hwint08:
    call real_clock_int
    call send_eoi
    iret
hwint12:
    call mouse_int
    call send_eoi
    iret
hwint13:
    call copr_int
    call send_eoi
    iret
hwint14:
    call harddisk_int
    call send_eoi
    iret
