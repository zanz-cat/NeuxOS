SELECTOR_KERNEL_CS  equ 1000b

extern gdt_ptr
extern idt_ptr
extern relocate_gdt
extern kernel_idle

extern init_interrupt
extern clear_screen

extern init_system

[SECTION .bss]
stack_space  resb    2 * 1024
stacktop: 

[SECTION .text]
global _start

_start:
    ; init kernel stack
    mov esp, stacktop

    ; move GDT to kernel
    sgdt [gdt_ptr]
    call relocate_gdt
    lgdt [gdt_ptr]

    ; init interrupt
    call init_interrupt
    lidt [idt_ptr]

    ; jmp with new GDT, make sure GDT correct
    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    ; init system
    call init_system

    ; enable interrupt
    sti

    ; main loop
.hlt:
    hlt
    jmp .hlt
