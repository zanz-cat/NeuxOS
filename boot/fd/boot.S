org 07c00h

%include "const.S"

jmp LABEL_BEGIN
nop

BS_OEMName            db      'Lee Link'
BPB_BytesPerSec       dw      CONST_BPB_BytesPerSec
BPB_SecPerClus        db      CONST_BPB_SecPerClus
BPB_RsvdSecCnt        dw      CONST_BPB_RsvdSecCnt
BPB_NumFATs           db      CONST_BPB_NumFATs
BPB_RootEntCnt        dw      CONST_BPB_RootEntCnt
BPB_TotSec            dw      CONST_BPB_TotSec
BPB_Media             db      CONST_BPB_Media
BPB_FATSz16           dw      CONST_BPB_FATSz16
BPB_SecPerTrk         dw      CONST_BPB_SecPerTrk
BPB_NumHeads          dw      CONST_BPB_NumHeads
BPB_HiddSec           dd      CONST_BPB_HiddSec
BPB_TotSec32          dd      CONST_BPB_TotSec32
BS_DrvNum             db      CONST_BS_DrvNum
BS_Reserved1          db      CONST_BS_Reserved1
BS_BootSig            db      CONST_BS_BootSig
BS_VolID              dd      CONST_BS_VolID
BS_VolLab             db      'LEELINKOS01'
BS_FileSysType        db      'FAT12   '

TopOfStack            equ     0ffffh


; 1. read root directory information to 
;    [LoaderBase:LoaderOffset] from floppy temporarily.
; 2. search and read LOADER.BIN to [LoaderBase:LoaderOffset] from floppy, 
;    will override root directory information which is useless.
; 3. jump to the LOADER
LABEL_BEGIN:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    ; init stack
    mov sp, TopOfStack

    ; compute root directory sector numbers
    mov al, [BPB_RootEntCnt]
    mov bl, CONST_FAT12_RootEntSize
    mul bl
    mov bx, [BPB_BytesPerSec]
    xor dx, dx
    div bx
    cmp dx, 0
    jz .a1
    inc ax
.a1:
    mov [RootDirSectorNum], ax

    ; compute FAT table base address
    mov ax, [BPB_BytesPerSec]
    mov bx, [BPB_FATSz16]
    mul bx 
    mov bx, 16  ; 16 is the step of base address
    div bx
    cmp dx, 0
    jz .a2
    inc ax
.a2:
    mov word [FATTableBase], LoaderBase
    sub [FATTableBase], ax

    ; reset floppy driver
    xor ah, ah
    xor dl, dl
    int 13h

    ; read root directory
    mov ax, LoaderBase
    mov es, ax
    mov bx, LoaderOffset
    mov ax, CONST_SectorOfRootDir
    mov cl, [RootDirSectorNum]
    call ReadSector

    ; read FAT table
    mov ax, [FATTableBase]
    mov es, ax
    xor bx, bx
    mov ax, 1
    mov cx, [BPB_FATSz16]
    call ReadSector

    ; read loader
    call SearchAndReadLoader

    ; display OK
    mov bp, OKMsg
    mov cx, OKMsgLen
    call DispStr

    jmp LoaderBase:LoaderOffset

; function: read secotr
; ax sector, start from 0
; cl numbers of sectors to read
; es:bx result buffer address
ReadSector:
    push bp
    mov bp, sp
    sub sp, 3

    mov dl, CONST_BPB_SecPerTrk
    div dl
    inc ah
    mov [bp-3], ax
    and ax, 0ffh
    mov dl, CONST_BPB_NumHeads
    div dl

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
    jc .error
    add sp, 3
    pop bp
    ret
.error:
    jmp $

; function: search file `LOADER.BIN`, 
; and copy it from floppy to memory.
SearchAndReadLoader:
    cld    
    mov si, LoaderName
    mov ax, LoaderBase
    mov es, ax
    mov di, LoaderOffset

    mov cx, [BPB_RootEntCnt]
.a1:
    push cx
    push si
    push di
    mov cx, LoaderNameLen
    repe cmpsb
    pop di
    pop si
    pop cx
    jz .a2     ; LOADER.BIN FOUND
    add di, CONST_FAT12_RootEntSize
    loop .a1

    ; print "loader not found" message
    mov bp, LoaderNotFoundMsg
    mov cx, LoaderNotFoundMsgLen
    call DispStr
    jmp $
.a2:
    mov ax, [es:di+0x1a]    ; FAT12 RootDirEntry.FirstCluster

    ; alloc local variable for bytes already read
    push bp
    mov bp, sp
    sub sp, 2
    mov word [bp-2], 0

    ; display boot message
    push ax
    push bp
    mov bp, BootMsg
    mov cx, BootMsgLen
    call DispStr
    pop bp
    pop ax

.readSector:
    ; ax is logical sector number
    push ax
    ; AbsCls = LogicCls - 2 + 19 + RootDirSectorNum
    add ax, 17
    add ax, [RootDirSectorNum]
    mov cl, 1
    mov bx, LoaderBase
    mov es, bx
    mov bx, LoaderOffset
    add bx, [bp-2]
    call ReadSector

    ; display dot
    push bp
    mov bp, DotStr
    mov cx, 1
    call DispStr
    pop bp

    pop ax
    call GetFATEntry
    cmp bx, 0xff7   ; bad sector
    jz .a3
    cmp bx, 0xff8   ; file end
    jnb .a4
    mov ax, bx
    add word [bp-2], CONST_BPB_BytesPerSec
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

   mov bx, [FATTableBase]
   mov es, bx
   mov bx, CONST_FAT12_BitsPerClus
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

; Display string at cursor position
; es:bp String address offset
; cx String length
DispStr:
   push cx
   
   mov ax, ds
   mov es, ax

   mov ah, 03h
   mov bh, 0
   int 10h

   mov ah, 13h
   mov al, 01h
   mov bh, 0h
   mov bl, 07h
   pop cx
   int 10h

   ret

BootMsg               db    'booting'
BootMsgLen            equ   $ - BootMsg

DotStr                db    '.'

LoaderNotFoundMsg     db    'loader not found!'
LoaderNotFoundMsgLen  equ   $ - LoaderNotFoundMsg

BadSectorMsg          db    'bad sector'
BadSectorMsgLen       equ   $ - BadSectorMsg

LoaderName            db    'LOADER  BIN'
LoaderNameLen         equ   $ - LoaderName

OKMsg                 db    'ok'
OKMsgLen              equ   $ - OKMsg

RootDirSectorNum      dw   0
FATTableBase        dw   0

times 510 - ($-$$) db 0
dw 0xaa55
