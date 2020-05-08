global out_byte
global in_byte

[SECTION .text]
; void out_byte(u16 port, u8 value);
out_byte:
    push ebp
    mov ebp, esp

    mov edx, [esp+8]
    mov al, [esp+12]
    out dx, al
    nop
    nop

    pop ebp
    ret

; u8 in_byte(u16 port);
in_byte:
    push ebp
    mov ebp, esp

    mov edx, [esp+8]
    in al, dx
    and eax, 0ffh
    nop
    nop

    pop ebp
    ret