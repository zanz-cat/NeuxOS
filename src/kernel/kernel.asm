SELECTOR_KERNEL_CS  equ 8

extern cstart
extern gdt_ptr

[SECTION .bss]
StackSpace  resb    2 * 1024
StackTop: 

[SECTION .text]
global _start

_start:
    mov esp, StackTop
    sgdt [gdt_ptr]    
    call cstart
    lgdt [gdt_ptr]

    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    ; xchg bx, bx
    
    mov ah, 0fh
    mov al, 'K'
    mov [gs:((80 * 1 + 39) * 2)], ax

    push 0
    popfd
    hlt
