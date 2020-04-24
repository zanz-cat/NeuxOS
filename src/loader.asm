org 0x100
jmp LABEL_START

%include "fat12hdr.inc"
%include "boot.inc"
%include "function.asm"
%include "loader.inc"

; GDT
LABEL_GDT:              Descriptor     0,          0,              0       ; dummy descriptor
LABEL_DESC_FLAT_C:      Descriptor     0,          0fffffh,        DA_CR|DA_32|DA_LIMIT_4K
LABEL_DESC_FLAT_RW:     Descriptor     0,          0fffffh,        DA_DRW|DA_32|DA_LIMIT_4K
LABEL_DESC_VIDEO:       Descriptor     0b8000h,    0ffffh,         DA_DRW|DA_DPL3

GdtLen                  equ $ - LABEL_GDT
GdtPtr                  dw  GdtLen - 1
                        dd  BaseOfLoaderPhyAddr + LABEL_GDT

; GDT Selector
SelectorFlatC           equ LABEL_DESC_FLAT_C - LABEL_GDT
SelectorFlatRW          equ LABEL_DESC_FLAT_RW - LABEL_GDT
SelectorVideo           equ LABEL_DESC_VIDEO - LABEL_GDT + SA_RPL3

TopOfStack              equ 0ffffh
BaseOfARDSBuffer        equ 07000h
ARDSNum                 dw  0
CursorPosition          dw  0
CursorPositionPhyAddr   equ BaseOfLoaderPhyAddr + CursorPosition

; the macro function get physical address...

; 1. Search and read kernel file to [BaseOfKernelFile:OffsetOfKernelFile]
;    during this step, will read Root Directory information from floppy 
;    to [BaseOfKernelFile:OffsetOfKernelFile], However the information will
;    be overwrite by kernel file after used.
; 2. enter protect mode
; 3. setup paging and enable paging
; 4. relocate kernel
; 5. Jump to kernel (^.^)

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov es, ax

    ; init stack
    mov sp, TopOfStack

    call DispEnter
    mov bp, LOADING_MESSAGE
    mov cx, LOADING_MESSAGE_LEN
    call DispStr
    call DispEnter

    ; read kernel
    mov ax, cs
    mov es, ax
    mov bp, KernelMsg
    mov cx, KernelMsgLen
    call DispStr
    call ReadKernel
    mov ax, cs
    mov es, ax
    mov bp, LoadedMsg
    mov cx, LoadedMsgLen
    call DispStr
    call DispEnter

    ; stop floppy motor
    call KillMotor

    ; read memory info
    call ReadMemInfo

    ; load GDT
    lgdt    [GdtPtr]
    
    ; close interrupt
    cli

    ; open A20 address line
    in  al, 92h
    or  al, 10b
    out 92h, al

    ; enable protect mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    ; jmp to 32bit segment
    jmp dword   SelectorFlatC:(BaseOfLoaderPhyAddr + LABEL_PM_START)

ReadKernel:
    ; compute root directory sector numbers
    mov al, CONST_BPB_RootEntCnt
    mov bl, 32
    mul bl
    mov bx, CONST_BPB_BytesPerSec
    xor dx, dx
    div bx
    cmp dx, 0
    jz .a1
    inc ax
.a1:
    mov [RootDirSectorNum], ax

    ; compute FAT table base address
    mov ax, CONST_BPB_BytesPerSec
    mov bx, CONST_BPB_FATSz16
    mul bx 
    mov bx, 16
    div bx
    cmp dx, 0
    jz .a2
    inc ax
.a2:
    mov word [BaseOfFATTable], BaseOfKernelFile
    sub [BaseOfFATTable], ax

    ; reset floppy driver
    xor ah, ah
    xor dl, dl
    int 13h

    ; read root directory
    mov ax, BaseOfKernelFile
    mov es, ax
    mov bx, OffsetOfKernelFile
    mov ax, CONST_SectorOfRootDir
    mov cl, [RootDirSectorNum]
    call ReadSector

    ; read FAT table
    mov ax, [BaseOfFATTable]
    mov es, ax
    xor bx, bx
    mov ax, 1
    mov cx, CONST_BPB_FATSz16
	call ReadSector

    ; read Kernel
    call SearchAndReadKernel

    ret

; function: read secotr
; ax sector, start from 0
; cl numbers of sectors to read
; es:bx result buffer address
ReadSector:
    push bp
    mov bp, sp
    push ax
    sub sp, 3

    mov dl, 18
    div dl
    inc ah
    mov [bp-3], ax
    and ax, 0ffh
    mov dl, 2
    div dl

.GoOnReading:
    mov dl, 0
    mov ch, al
    mov dh, ah
    mov ax, [bp-3]
    shr ax, 8
    mov [bp-1], al
    mov al, cl
    mov cl, [bp-1]
    mov ah, 02h
    int 13h
    jc .GoOnReading

    add sp, 3
    pop ax
    mov sp, bp
    pop bp
    ret

; function: search kernel file, 
; and copy it from floppy to memory.
SearchAndReadKernel:
    cld    
    mov si, KernelName
    mov ax, BaseOfKernelFile
    mov es, ax
    mov di, OffsetOfKernelFile

    mov cx, CONST_BPB_RootEntCnt
.a1:
    push cx
    push si
    push di
    mov cx, KernelNameLen
    repe cmpsb
    pop di
    pop si
    pop cx
    jz .a2     ; FOUND
    add di, 32
    loop .a1

    ; print "Kernel not found" message
    mov ax, cs
    mov es, ax
    mov bp, NotFoundMsg
    mov cx, NotFoundMsgLen
    call DispStr
    jmp $
    
.a2:
    mov ax, [es:di+0x1a]

    ; alloc local variable for bytes already read
    push bp
    mov bp, sp
    sub sp, 2
    mov word [bp-2], 0

.readSector:
    ; ax is logical sector number
    push ax
    ; AbsCls = LogicCls - 2 + 19 + RootDirSectorNum
    add ax, 17
    add ax, [RootDirSectorNum]
    mov cl, 1
    mov bx, BaseOfKernelFile
    mov es, bx
    mov bx, OffsetOfKernelFile
    add bx, [bp-2]
    call ReadSector

    ; display dot
    push bp
    mov ax, cs
    mov es, ax
    mov bp, DotStr
    mov cx, 1
    call DispStr
    pop bp

    pop ax
    call GetFATEntry
    cmp bx, 0xff7
    jz .a3
    cmp bx, 0xff8
    jnb .a4
    mov ax, bx
    add word [bp-2], 512
    jmp .readSector
.a3:
    mov bp, BadSectorMsg
    mov cx, BadSectorMsgLen
    call DispStr
    jmp $

.a4:
    add  sp, 2
    pop bp
    ret

; input: ax logical sector number
; return: bx FAT value, next logical sector number
GetFATEntry:
    push si
    push ax
 
    mov bx, [BaseOfFATTable]
    mov es, bx
    mov bx, 12
    mul bx
    mov bx, 8
    div bx
    mov si, ax
    mov bx, [es:si]
    cmp dx, 0
    jz .a1
    shr bx, 4
    jmp .a2
.a1:
    and bx, 0fffh

.a2:
    pop ax
    pop si
    ret

ReadMemInfo:
    push ebp
    mov ebp, esp

    sub esp, 2
    mov word [ebp-2], 0

    mov ebx, 0
.continue:
    mov ax, BaseOfARDSBuffer
    mov es, ax
    mov di, [ebp-2]
    mov ax, 0e820h
    mov ecx, 20
    mov edx, 0534d4150h
    int 15h
    jc .continue    ; cf = 1, 出错重试
    ; success
    add word [ebp-2], 20
    inc word [ARDSNum]
    ; check if the last one
    cmp ebx, 0
    jnz .continue ; not the last one

    add esp, 2
    pop ebp
    ret

[SECTION .s32]
ALIGN 32
[BITS 32]
LABEL_PM_START:
    mov ax, SelectorFlatRW
    mov ds, ax
    mov es, ax

    ; init stack
    mov ss, ax
    mov esp, TopOfStack    

    mov ax, SelectorVideo
    mov gs, ax

    ; get cursor position
    call GetCursor

    ; setup paging
    call SetupPaging

    ; relocate kernel

    ; jmp to kernel
    jmp $

SetupPaging:
    call DisplayARDS
    ; 获取系统可用内存
    ; 初始化页目录
    ; 初始化页表
    ; 开启分页机制
    ret

; 打印ARDS
DisplayARDS:
    mov ecx, 10
.next:
    push ecx

    mov ecx, MemoryInfoMsgLen
    mov esi, BaseOfLoaderPhyAddr + MemoryInfoMsg
    call DisplayStr
    call DisplayEnter
    call sleep

    pop ecx
    loop .next
    ret

;----------------------------------------------------------------------------
; 函数名: DispDigitalHex
;----------------------------------------------------------------------------
; 作用:
;   打印eax寄存器中的数字的16进制
;   入参: eax
DispDigitalHex:
    push eax
    push ebp
    mov ebp, esp
    sub esp, 4   
       
    mov [ebp-4], eax

    mov cx, 4
.print_number:
    mov esi, ebp
    add si, cx
    sub si, 5
    mov byte al, [ss:si]
    shr al, 4
    call .number2Char
    call DisplayAL

    mov byte al, [ss:si]
    call .number2Char
    call DisplayAL
    loop .print_number
 
    add esp, 4
    pop ebp
    pop eax
    ret

.number2Char:
    and al, 0fh
    cmp al, 10
    jnb .b
    add al, '0'
    ret
.b:
    sub al, 10
    add al, 'a'
    ret 

;----------------------------------------------------------------------------
; 函数名: DisplayAL
;----------------------------------------------------------------------------
; 作用:
;   打印AL中的字符
;   入参: al
DisplayAL:
    push ax

    mov ah, 07h
    mov di, [CursorPositionPhyAddr]
    shl di, 1
    mov [gs:di], ax
    
    inc word [CursorPositionPhyAddr]
    call SetCursor

    pop ax
    ret

Display0x:
    mov al, '0'
    call DisplayAL
    mov al, 'x'
    call DisplayAL

; FIXME: 非0时才设置空字符
SetCursor:
    push ax

    mov di, [CursorPositionPhyAddr]
    shl di, 1
    mov ah, 07h
    mov al, ' '
    mov [gs:di], ax

    mov dx, 0x3d4
    mov al, 0eh
    out dx, al
    mov dx, 0x3d5
    mov ax, [CursorPositionPhyAddr]
    mov al, ah
    out dx, al

    mov dx, 0x3d4
    mov al, 0fh
    out dx, al
    mov dx, 0x3d5
    mov ax, [CursorPositionPhyAddr]
    out dx, al

    pop ax
    ret

GetCursor:
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

    mov [CursorPositionPhyAddr],  bx
    ret

;----------------------------------------------------------------------------
; 函数名: DisplaySpace
;----------------------------------------------------------------------------
; 作用:
;   打印空格
DisplaySpace:
    push ax
    mov al, ' '
    call DisplayAL
    pop ax
    ret

; ecx: string length
; esi: address of string
; FIXME
DisplayStr:
.next:
    lodsb
    call DisplayAL
    loop .next
    ret

DisplayEnter:
    mov dx, [CursorPositionPhyAddr]
    mov ax, dx
    mov bl, 80
    div bl
    shr ax, 8
    sub dx, ax
    add dx, 80
    mov [CursorPositionPhyAddr], dx
    cmp dx, 80*25
    je  .scrollup
    call SetCursor
    ret
.scrollup:
    call ScrollUpScreen
    ret

    ret

ClearScreen:
    mov ax, gs
    mov es, ax
    xor edi, edi
    xor ax, ax
    mov cx, 80 * 25
    rep stosw
    
    mov word [CursorPositionPhyAddr], 0
    call SetCursor
    ret

ScrollUpScreen:
    push ecx

    mov ax, gs
    mov ds, ax
    mov es, ax
    mov esi, 80*2
    mov edi, 0
    mov cx, 80 * (25-1)
    rep movsw

    ; clear the bottom line
    xor ax, ax
    mov cx, 80
    rep stosw

    ; restore ds
    mov ax, SelectorFlatRW
    mov ds, ax
    sub word [CursorPositionPhyAddr], 80
    call SetCursor

    pop ecx
    ret

sleep:
    push ecx
    mov ecx, 02ffffffh
.next:
    loop .next
    pop ecx
    ret

[SECTION .msg]
LOADING_MESSAGE      db  'Loading...'
LOADING_MESSAGE_LEN  equ $ - LOADING_MESSAGE

RootDirSectorNum     dw 0
BaseOfFATTable       dw 0 

KernelName           db 'KERNEL  ELF'
KernelNameLen        equ $ - KernelName

NotFoundMsg          db ' Not Found!'
NotFoundMsgLen       equ $ - NotFoundMsg

BadSectorMsg         db 'Bad Sector!'
BadSectorMsgLen      equ $ - BadSectorMsg

KernelMsg            db 'Kernel'
KernelMsgLen         equ $ - KernelMsg

LoadedMsg            db 'Loaded'
LoadedMsgLen         equ $ - LoadedMsg

MemoryInfoMsg        db 'Memory Information'
MemoryInfoMsgLen     equ $ - MemoryInfoMsg

DotStr               db    '.'
