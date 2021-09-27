%include "const.S"
%include "config.S"

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

    ; can't uninstall 0~1MB page table, which is needed by 'jmp tss'
    ; mov dword [CONFIG_KERNEL_VMA+CONFIG_KERNEL_PG_ADDR], 0

    ; jmp with new GDT, make sure GDT correct
    jmp SELECTOR_KERNEL_CS:csinit
csinit:
    mov ax, SELECTOR_KERNEL_DS
    mov ds, ax

    ; setup kernel
    call kernel_setup

    ; load and jmp to kernel_loop task
    mov eax, [current]
    jmp far [eax]