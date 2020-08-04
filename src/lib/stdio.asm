%include "include/common.inc"

global out_byte
global in_byte

[SECTION .data]
cursor_pos  dw  -1

[SECTION .text]
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
