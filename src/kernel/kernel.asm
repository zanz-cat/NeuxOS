%include "include/common.inc"

SELECTOR_KERNEL_CS  equ 1000b

global _start
global sys_stacktop
global kernel_idle

extern gdt_ptr
extern idt_ptr
extern init_gdt
extern init_interrupt
extern clear_screen
extern init_system
extern _idle
extern tss_sel
extern current

[SECTION .bss]
stack_space  resb    2 * 1024
sys_stacktop: 

[SECTION .text]
_start:
    ; init system stack
    mov esp, sys_stacktop

    ; move GDT to kernel
    sgdt [gdt_ptr]
    call init_gdt
    lgdt [gdt_ptr]

    ; init interrupt
    call init_interrupt
    lidt [idt_ptr]

    ; jmp with new GDT, make sure GDT correct
    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    ; init system
    call init_system
    cmp eax, 0
    je  .continue
    hlt
.continue:
    ; load tss
    ltr word [tss_sel]

    ; enable interrupt
    ; sti   ; not neccessary because iret will enable interrupt

    ; load and jmp to idle proc
    mov eax, [current]
    lldt word [eax+1100]
    mov esp, [eax+15*4]
    mov eax, [eax+16*4]
    mov ss, ax
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret

kernel_idle:
    call _idle
    hlt
    jmp kernel_idle