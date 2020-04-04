org 0x100
jmp LABEL_START

%include "fat12hdr.inc"
%include "boot.inc"
%include "function.asm"
%include "loader.inc"

; GDT
LABEL_GDT:           Descriptor     0,          0,              0       ; dummy descriptor
LABEL_DESC_FLAT_C:   Descriptor     0,          0fffffh,        DA_CR|DA_32|DA_LIMIT_4K
LABEL_DESC_FLAT_RW:  Descriptor     0,          0fffffh,        DA_DRW|DA_32|DA_LIMIT_4K
LABEL_DESC_VIDEO:    Descriptor     0b8000h,    0ffffh,         DA_DRW|DA_DPL3

GdtLen               equ $ - LABEL_GDT
GdtPtr               dw  GdtLen - 1
                     dd  BaseOfLoaderPhyAddr + LABEL_GDT

; GDT Selector
SelectorFlatC        equ LABEL_DESC_FLAT_C - LABEL_GDT
SelectorFlatRW       equ LABEL_DESC_FLAT_RW - LABEL_GDT
SelectorVideo        equ LABEL_DESC_VIDEO - LABEL_GDT + SA_RPL3

; 1. Search and read kernel file to [BaseOfKernelFile:OffsetOfKernelFile]
;    during this step, will read Root Directory information from floppy 
;    to [BaseOfKernelFile:OffsetOfKernelFile], However the information will
;    be overwrite by kernel file later.
; 2. enter protect mode
; 3. setup paging and enable paging
; 4. relocate kernel
; 5. Jump to kernel (^.^)

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov es, ax

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

    ; stop floppy motor
    call KillMotor

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
    jc    .GoOnReading

    add sp, 3
    pop ax
    mov sp, bp
    pop bp
    ret

; function: search file `KERNEL.BIN`, 
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


[SECTION .s32]
ALIGN 32
[BITS 32]
LABEL_PM_START:
    ; setup paging

    ; enable paging

    ; relocate kernel

    ; jmp to kernel
    jmp $


[SECTION .msg]
LOADING_MESSAGE      db  'Loading...'
LOADING_MESSAGE_LEN  equ $ - LOADING_MESSAGE

RootDirSectorNum     dw 0
BaseOfFATTable       dw 0 

KernelName           db 'KERNEL  BIN'
KernelNameLen        equ $ - KernelName

NotFoundMsg    db ' Not Found!'
NotFoundMsgLen equ $ - NotFoundMsg

BadSectorMsg         db 'Bad Sector!'
BadSectorMsgLen      equ $ - BadSectorMsg

KernelMsg            db 'Kernel'
KernelMsgLen         equ $ - KernelMsg

LoadedMsg            db 'Loaded.'
LoadedMsgLen         equ $ - LoadedMsg

DotStr               db    '.'
