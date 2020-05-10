SELECTOR_KERNEL_CS  equ 1000b

extern gdt_ptr
extern idt_ptr
extern init_8259A
extern relocate_gdt
extern init_idt
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

    ; jmp with new GDT, make sure GDT correct
    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    ; init interrupt
    call init_8259A
    call init_idt
    lidt [idt_ptr]

    call kernel_started

    push 0
    popfd
    hlt

