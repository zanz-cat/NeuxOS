; 名称：divdw
; 功能：进行不会产生溢出的除法运算，被除数为dword型，除数为word型号，结果为dword型号
; 参数：(ax)=dword型数据的低16位
;      (dx)=dword型数据的高16位
;      (cx)=除数
; 返回：(dx)=结果的高16位，(ax)=结果的低16位
;      (cx)=余数
divdw:    ; 子程序定义开始
    push bp
    mov bp, sp
    push bx

    push ax            ; 低16位先保存
    mov ax, dx         ; ax值为高16为
    mov dx, 0          ; dx置零
    div cx             ; H/N
    mov bx, ax         ; ax,bx的值(int)H/N，dx的值为(rem)H/N
    pop ax             ; 处理低16位
    div cx             ; 高16位:dx的值为(rem)H/N，低16位:ax
    mov cx, dx
    mov dx, bx

    pop bx
    leave
    ret

; 参数：(ax)=dword型数据的低16位
;      (dx)=dword型数据的高16位
;      (cx)=乘数
; 丢掉溢出位
muldw:
    push bp
    mov bp, sp

    sub sp, 6
    mov [bp-2], dx

    mul cx
    mov [bp-4], dx
    mov [bp-6], ax
    mov ax, [bp-2]
    mul cx
    mov dx, ax
    add dx, [bp-4]
    mov ax, [bp-6]

    leave
    ret
