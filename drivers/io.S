%define arg(i) [ebp+4*(i+2)]

global out_byte
global in_byte
global in_word

; void out_byte(uint16_t port, uint8_t value);
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

; uint8_t in_byte(uint16_t port);
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

; uint16_t in_word(uint16_t port);
in_word:
    push ebp
    mov ebp, esp
    push edx

    mov edx, arg(0)
    xor eax, eax
    in ax, dx
    nop
    nop
    nop
    nop

    pop edx
    pop ebp
    ret
