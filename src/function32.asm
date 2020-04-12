;----------------------------------------------------------------------------
; 函数名: DispDigitalHex
;----------------------------------------------------------------------------
; 作用:
;   打印eax寄存器中的数字的16进制
DispDigitalHex:
    push eax
    push ebp
    mov ebp, esp
    sub esp, 4   
   
    mov [ebp-4], eax
    mov al, [ebp-4]
    mov al, [ebp-3]
    mov al, [ebp-2]
    mov al, [ebp-1]
    
    mov ah, 09h
    mov al, '1'
    xchg bx, bx
    int 10h
    xchg bx, bx
 
    add esp, 4
    pop ebp
    pop eax
    ret
