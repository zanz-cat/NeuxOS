extern exception_handler
extern send_eoi

extern clock_int_handler
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

extern current

[SECTION .text]

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
    push ds
    push es
    mov ax, 10h
    mov ds, ax
    mov es, ax

    cmp dword [current], 0
    je  .skip
    ; the proc interrupted
    mov edi, [current]
    ; save eflags
    mov eax, [esp+16]
    mov [edi+8], eax
    ; save cs and eip
    ; TODO
.skip:
    call clock_int_handler
    push eax
    call send_eoi
    pop esi

    ; init new proc   TODO: move to c code
    mov eax, [esi+8]
    mov [esi+1036-4], eax

    xor eax, eax
    mov ax, cs
    mov [esi+1036-8], eax

    mov eax, [esi+4]
    mov [esi+1036-12], eax

    pop es
    pop ds

    ; switch stack
    lea esp, [esi+1036-12]
    iret
hwint01:
    call keyboard_int
    call send_eoi
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
