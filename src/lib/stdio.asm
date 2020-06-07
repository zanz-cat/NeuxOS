%include "include/common.inc"

global _backspace
global _putchar
global _putchar_pos
global out_byte
global in_byte
global clear_screen

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
    push eax
    push edx

    mov edx, arg(0)
    in al, dx
    and eax, 0ffh
    nop
    nop
    nop
    nop

    pop edx
    pop eax
    pop ebp
    ret

; void _backspace()
_backspace:
    push ebp
    mov ebp, esp
    push edi

    cmp word [cursor_pos], -1
    jne .skip
    call get_cursor
.skip:
    dec word [cursor_pos]
    call set_cursor
    mov di, [cursor_pos]
    shl di, 1
    mov word [gs:di], 0700h

    pop edi
    pop ebp
    ret

; u32 _putchar(u32 ch, u8 color)
_putchar:
    push ebp
    mov ebp, esp
    push edx
    push ebx

    cmp word [cursor_pos], -1
    jne .skip
    call get_cursor
.skip:
    mov eax, arg(0)
    cmp al, 10 ; newline
    jne .1
    ; new line
    mov dx, [cursor_pos]
    mov ax, dx
    mov bl, 80
    div bl
    shr ax, 8
    sub dx, ax
    add dx, 80
    mov [cursor_pos], dx
    cmp dx, 80*25
    je  .scrollup
    call set_cursor
    jmp .out
.scrollup:
    call scroll_up_screen
    jmp .out

.1:
    mov ah, arg(1)
    mov di, [cursor_pos]
    shl di, 1
    mov [gs:di], ax
    inc word [cursor_pos]
    call set_cursor

.out:
    mov eax, arg(0)
    pop ebx
    pop edx
    pop ebp
    ret

; u32 _putchar_pos(u32 ch, u16 pos, u8 color)
_putchar_pos:
    push ebp
    mov ebp, esp

    mov eax, arg(0)
    mov ah, arg(2)
    mov di, arg(1)
    shl di, 1
    mov [gs:di], ax

    mov eax, arg(0)
    pop ebp
    ret

set_cursor:
    push ax
    push dx

    mov dx, 0x3d4
    mov al, 0eh
    out dx, al
    mov dx, 0x3d5
    mov ax, [cursor_pos]
    mov al, ah
    out dx, al

    mov dx, 0x3d4
    mov al, 0fh
    out dx, al
    mov dx, 0x3d5
    mov ax, [cursor_pos]
    out dx, al

    pop dx
    pop ax
    ret

get_cursor:
    push ax
    push bx
    push dx

    mov dx, 0x3d4
    mov al, 0eh
    out dx, al
    mov dx, 0x3d5
    in al, dx
    mov bh, al

    mov dx, 0x3d4
    mov al, 0fh
    out dx, al
    mov dx, 0x3d5
    in al, dx
    mov bl, al

    mov [cursor_pos],  bx

    pop dx
    pop bx    
    pop ax
    ret

scroll_up_screen:
    push ecx    
    push ds
    push es

    mov ax, gs
    mov ds, ax
    mov es, ax
    mov esi, 80*2
    mov edi, 0
    mov ecx, 80 * (25-1)
    rep movsw

    ; reset the bottom line
    mov ax, 0700h
    mov ecx, 80
    rep stosw

    ; restore ds and es
    pop es
    pop ds

    sub word [cursor_pos], 80
    call set_cursor

    pop ecx
    ret

clear_screen:
    push es
    push eax
    push edi
    push ecx

    mov ax, gs
    mov es, ax
    xor edi, edi
    xor ax, ax
    mov ecx, 80 * 25
    rep stosw
    
    mov word [cursor_pos], 0
    call set_cursor
    xor eax, eax

    pop ecx
    pop edi
    pop eax
    pop es
    ret