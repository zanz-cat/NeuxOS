%include "common.S"

global _start
global out_byte
global in_byte

extern current
extern init_gdt
extern init_system

[SECTION .bss]
resb    1024
temp_stacktop: 

[SECTION .text]
_start:
    ; init system stack
    mov esp, temp_stacktop

    ; move GDT to kernel
    call init_gdt

    ; jmp with new GDT, make sure GDT correct
    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    ; init system
    call init_system

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

; void out_byte(u16 port, u8 value);
out_byte:
    push ebp
    mov ebp, esp
    push eax
    push edx

    mov edx, arg(0)
    mov al, arg(1)
    out dx, al
    nop
    nop
    nop
    nop

    pop edx
    pop eax
    pop ebp
    ret

; u8 in_byte(u16 port);
in_byte:
    push ebp
    mov ebp, esp
    push edx

    mov edx, arg(0)
    xor eax, eax
    in al, dx
    nop
    nop
    nop
    nop

    pop edx
    pop ebp
    ret
