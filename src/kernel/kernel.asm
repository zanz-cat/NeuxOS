SELECTOR_KERNEL_CS  equ 1000b

extern gdt_ptr
extern idt_ptr
extern relocate_gdt
extern init_interrupt
extern kernel_started

[SECTION .bss]
StackSpace  resb    2 * 1024
StackTop: 

[SECTION .text]
global _start

_start:
    ; init kernel stack
    mov esp, StackTop

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
    call kernel_started
    sti
    hlt
