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

    ; enable interrupt
    ; sti   ; not neccessary because iret will enable interrupt

    ; load and jmp to idle proc
    mov eax, [current]
    lldt word [eax+OFFSET_PROC_LDT_SEL]
    mov esp, [eax+OFFSET_PROC_ESP0]
    mov ss, [eax+OFFSET_PROC_SS0]
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iret
