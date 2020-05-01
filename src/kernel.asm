SELECTOR_KERNEL_CS  equ 8

extern cstart
extern gdt_ptr

[section .bss]
StackSpace  resb    2 * 1024
StackTop: 

[section .text]
global _start

_start:
    mov ah, 0fh
    mov al, 'K'
    mov [gs:((80 * 1 + 39) * 2)], ax

    mov esp, StackTop
    sgdt [gdt_ptr]
    call cstart
    lgdt [gdt_ptr]

    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    push 0
    popfd
    hlt
