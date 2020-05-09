SELECTOR_KERNEL_CS  equ 1000b

extern cstart
extern gdt_ptr
extern idt_ptr
extern init_8259A

[SECTION .bss]
StackSpace  resb    2 * 1024
StackTop: 

[SECTION .text]
global _start

_start:
    mov esp, StackTop
    call init_8259A
    sgdt [gdt_ptr]
    sidt [idt_ptr]
    call cstart
    lgdt [gdt_ptr]
    lidt [idt_ptr]

    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    mov ah, 0fh
    mov al, 'K'
    mov [gs:((80 * 1 + 39) * 2)], ax

    push 0
    popfd
    hlt
