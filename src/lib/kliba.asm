global out_byte
global in_byte

[SECTION .text]
; void out_byte(u16 port, u8 value);
out_byte:
    mov edx, [esp+4]
    mov al, [esp+8]
    out dx, al
    nop
    nop
    ret

; u8 in_byte(u16 port);
in_byte:
    mov edx, [esp+4]
    in al, dx
    and eax, 0ffh
    nop
    nop
    ret