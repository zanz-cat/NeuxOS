%include "const.S"

global _start

extern out_byte
extern in_byte
extern current
extern gdt_init
extern kernel_init

[SECTION .bss]
resb    10240
temp_stacktop:

[SECTION .text]
_start:
    ; init temp kernel stack
    mov esp, temp_stacktop

    ; move GDT to kernel
    call gdt_init

    ; jmp with new GDT, make sure GDT correct
    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    ; init kernel
    call kernel_init

    ; load and jmp to idle task
    mov eax, [current]
    jmp far [eax+OFFSET_TASK_TSS_SEL-4]