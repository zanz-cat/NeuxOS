%include "const.S"

global _start

extern out_byte
extern in_byte
extern current
extern gdt_setup
extern kernel_setup

[SECTION .bss]
resb    10240
temp_stacktop:

[SECTION .text]
_start:
    ; init temp kernel stack
    mov esp, temp_stacktop

    ; move GDT to kernel
    call gdt_setup

    ; jmp with new GDT, make sure GDT correct
    jmp SELECTOR_KERNEL_CS:csinit
csinit:
    ; setup kernel
    call kernel_setup

    ; load and jmp to kernel_idle task
    mov eax, [current]
    jmp far [eax]