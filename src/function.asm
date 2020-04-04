; Display string at cursor position
; es:bp String address offset
; cx String length
DispStr:
   push ax
   push bx
   push cx
   push dx
   push cx

   mov ah, 03h
   mov bh, 0
   int 10h

   mov ah, 13h
   mov al, 01h
   mov bh, 0h
   mov bl, 07h
   pop cx
   int 10h

   pop dx
   pop cx
   pop bx
   pop ax
   ret

DispEnter:
   push ax
   push bx
   push cx
   push dx

   mov ah, 03h
   mov bh, 0
   int 10h
   
   mov ah, 02h
   mov bh, 0
   inc dh
   mov dl, 0
   int 10h

   pop dx
   pop cx
   pop bx
   pop ax
   ret

CleanScreen:
   mov ah, 06h
   mov al, 0h
   mov bh, 07h
   mov ch, 0
   mov cl, 0
   mov dh, 24
   mov dl, 79
   int 10h
   ret

   ; hide cursor
   ;mov ah, 01h
   ;mov cx, 2607h ; https://blog.csdn.net/qq_40818798/article/details/83933827
   ;int 10h
   ;ret

;----------------------------------------------------------------------------
; 函数名: KillMotor
;----------------------------------------------------------------------------
; 作用:
;   关闭软驱马达
KillMotor:
    push    dx
    mov dx, 03F2h
    mov al, 0
    out dx, al
    pop dx
    ret
