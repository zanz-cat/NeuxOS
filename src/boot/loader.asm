org 0x100
jmp LABEL_START

%include "boot/fat12hdr.inc"
%include "boot/boot.inc"
%include "boot/loader.inc"

%define paddr(b, o) (b << 4) + o

%macro callPrintStr 1
    push ecx
    mov ecx, %1Len
    mov esi, paddr(%1)
    call PrintStr
    pop ecx
%endmacro

%macro callPrintStrln 1
    callPrintStr %1
    call Println
%endmacro

%macro callPrintChar 0-2 al, 1
    push ecx
    %if %1 != al
        mov al, %1
    %endif
    mov ecx, %2
    call PrintAL
    pop ecx
%endmacro

; GDT
LABEL_GDT:              Descriptor     0,          0,              0       ; dummy descriptor
LABEL_DESC_FLAT_C:      Descriptor     0,          0fffffh,        DA_CR|DA_32|DA_LIMIT_4K
LABEL_DESC_FLAT_RW:     Descriptor     0,          0fffffh,        DA_DRW|DA_32|DA_LIMIT_4K
LABEL_DESC_VIDEO:       Descriptor     0b8000h,    0ffffh,         DA_DRW|DA_DPL3

GdtLen                  equ $ - LABEL_GDT
GdtPtr                  dw  GdtLen - 1
                        dd  paddr(LABEL_GDT)

; GDT Selector
SelectorFlatC           equ LABEL_DESC_FLAT_C - LABEL_GDT
SelectorFlatRW          equ LABEL_DESC_FLAT_RW - LABEL_GDT
SelectorVideo           equ LABEL_DESC_VIDEO - LABEL_GDT + SA_RPL3

ADDR_MSG_GAP_LEN        equ 12
LENGTH_MSG_GAP_LEN      equ 13
TopOfStack              equ 0ffffh
BaseOfARDSBuffer        equ 07000h
KernelFilePhyAddr       equ paddr(BaseOfKernelFile, OffsetOfKernelFile)
RootDirSectorNum        dw 0
BaseOfFATTable          dw 0 
ARDSNum                 dw 0
CursorPosition          dw 0
KernelEntryPointAddr    dd 0
MemSizeMB               dd 0

; Strings
LOADING_MESSAGE             db  'Start to load'
LOADING_MESSAGE_LEN         equ $ - LOADING_MESSAGE

OKMsg                       db    'ok'
OKMsgLen                    equ   $ - OKMsg

KernelName                  db 'KERNEL  ELF'
KernelNameLen               equ $ - KernelName

NotFoundMsg                 db ' Not Found!'
NotFoundMsgLen              equ $ - NotFoundMsg

BadSectorMsg                db 'Bad Sector!'
BadSectorMsgLen             equ $ - BadSectorMsg

KernelMsg                   db 'Kernel'
KernelMsgLen                equ $ - KernelMsg

LoadedMsg                   db 'loaded'
LoadedMsgLen                equ $ - LoadedMsg

MemInfoMsg                  db 'Memory Information'
MemInfoMsgLen               equ $ - MemInfoMsg

StartKernelMsg              db 'Start kernel up'
StartKernelMsgLen           equ $ - StartKernelMsg

MemBlockAddrMsg             db 'Address'
MemBlockAddrMsgLen          equ $ - MemBlockAddrMsg

MemBlockLengthMsg           db 'Length'
MemBlockLengthMsgLen        equ $ - MemBlockLengthMsg

MemBlockTypeMsg             db 'Type'
MemBlockTypeMsgLen          equ $ - MemBlockTypeMsg

InvalidKernelFileMsg        db 'Invalid Kernel File!'
InvalidKernelFileMsgLen     equ $ - InvalidKernelFileMsg

SetupPagingMsg              db 'Setup paging...'
SetupPagingMsgLen           equ $ - SetupPagingMsg

ProgramCopiedMsg            db ' program(s) copied'
ProgramCopiedMsgLen         equ $ - ProgramCopiedMsg

NOBITSInitMsg               db ' NOBITS(s) initialized'
NOBITSInitMsgLen            equ $ - NOBITSInitMsg

DotStr                      db    '.'


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
    mov bp, KernelMsg
    mov cx, KernelMsgLen
    call DispStr
    call ReadKernel
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
    jmp dword   SelectorFlatC:(paddr(LABEL_PM_START))

ReadKernel:
    push es
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

    pop es
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

[SECTION .s32]
BITS 32
ALIGN 32
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

    ; display memory info
    call DisplayARDS

    ; setup paging
    callPrintStr SetupPagingMsg
    call SetupPaging

    ; enable paging
    mov eax, PageDirBasePhyAddr
    mov cr3, eax
    mov eax, cr0
    or  eax, 80000000h
    mov cr0, eax
    callPrintStrln OKMsg    

    ; relocate kernel
    call RelocateKernel    

    ; jmp to kernel
    callPrintStrln StartKernelMsg
    jmp [paddr(KernelEntryPointAddr)]

RelocateKernel:
    push ebp
    mov ebp, esp
    sub esp, 2

    cmp dword [KernelFilePhyAddr], 0x464c457f ; 0x7f+'ELF'
    je  .continue
    callPrintStrln InvalidKernelFileMsg
    jmp $

    ; handle program headers
.continue:    
    xor ecx, ecx
    mov word [ebp-2], 0
    mov cx, [KernelFilePhyAddr+44] ; program header number
.next:
    push ecx
    mov ax, [KernelFilePhyAddr+44]
    sub ax, cx
    mul word [KernelFilePhyAddr+42] ; program header entity size
    and eax, 0ffh
    shl edx, 16
    add eax, edx
    mov esi, [KernelFilePhyAddr+28] ; program header table offset
    add esi, eax
    add esi, KernelFilePhyAddr ; esi is the absolute address of certain program header in memory

    mov ecx, [esi+16] ; program size
    mov edi, [esi+8] ; p_vaddr in memory
    mov esi, [esi+4] ; p_offset in file
    add esi, KernelFilePhyAddr ; absolute address in memmory
    ; mov ax, ds
    ; mov es, ax    
    rep movsb
    inc word [ebp-2]
    pop ecx
    loop .next

    mov ax, [ebp-2]
    call Digital2Char
    callPrintChar
    callPrintStrln ProgramCopiedMsg

    ; handle section: initialize .bss section
    xor ecx, ecx
    mov word [ebp-2], 0
    mov cx, [KernelFilePhyAddr+48] ; section header number
.nextb:
    push ecx
    mov ax, [KernelFilePhyAddr+48]
    sub ax, cx
    mul word [KernelFilePhyAddr+46] ; section header entity size
    and eax, 0ffh
    shl edx, 16
    add eax, edx
    mov esi, [KernelFilePhyAddr+32] ; section header table offset
    add esi, eax
    add esi, KernelFilePhyAddr
    cmp dword [esi+4], 8 ; NOBITS type
    jne .skip
    ; clear NOBITS type
    mov ecx, [esi+20]
    mov edi, [esi+12]
    mov al, 0
    rep stosb
    inc word [ebp-2]
.skip:
    pop ecx
    loop .nextb

    mov ax, [ebp-2]
    call Digital2Char
    callPrintChar
    callPrintStrln NOBITSInitMsg

    ; entry point
    mov eax, [KernelFilePhyAddr+24]
    mov [paddr(KernelEntryPointAddr)], eax

    add esp, 2
    pop ebp
    ret

SetupPaging:
    push ebp
    mov ebp, esp
    sub esp, 8
    
    ; 获取系统可用内存
    call GetTotalMemSize

    ; compute the number of page table required
    mov ax, [paddr(MemSizeMB)]
    mov dx, [paddr(MemSizeMB+2)]
    mov bx, 4 ; a page table maps to 4MB memory
    div bx
    test dx, 0ffffh
    jz  .skip
    inc ax
.skip:
    mov [ebp-2], ax
    mov word [ebp-4], 0
.loopa:
    ; 初始化页目录
    xor eax, eax
    mov ax, [ebp-4]
    shl eax, 12     ; AddrOfPageTable = i << 12 + PageTableBasePhyAddr
    add eax, PageTableBasePhyAddr
    mov ebx, eax
    or  ebx, PG_P | PG_USU | PG_RWW

    xor eax, eax
    mov ax, [ebp-4]
    shl eax, 2  ; mul 4    
    add eax, PageDirBasePhyAddr

    mov [eax], ebx

    ; 初始化页表
    and ebx, 0fffff000h
    mov word [ebp-6], 1024
    mov word [ebp-8], 0
.loopb:
    ; AddrOfPage = i << 22 + j << 12
    xor eax, eax
    xor edx, edx
    mov ax, [ebp-4]
    mov dx, [ebp-8]
    shl eax, 22
    shl edx, 12
    add eax, edx
    or  eax, PG_P | PG_USU | PG_RWW
    mov [ebx], eax

    add ebx, 4
    inc word [ebp-8]
    mov ax, [ebp-6]
    cmp ax, [ebp-8]
    jne .loopb

    inc word [ebp-4]
    mov ax, [ebp-2]
    cmp ax, [ebp-4]
    jne .loopa

    add esp, 8
    pop ebp
    ret

GetTotalMemSize:
    xor ecx, ecx
    mov cx, [paddr(ARDSNum)]
.next:
    push ecx
    xor eax, eax
    mov ax, [paddr(ARDSNum)]
    sub ax, cx
    mov bl, 20
    mul bl
    mov esi, paddr(BaseOfARDSBuffer, 0)
    add esi, eax

    ; only available memory
    ; mov al, [esi+16]
    ; and al, 0fh
    ; cmp al, 0x1
    ; jne .label

    mov eax, [esi+8]
    add [paddr(MemSizeMB)], eax
    pop ecx
    loop .next

    ; transfer to MB
    mov eax, [paddr(MemSizeMB)]
    shr dword [paddr(MemSizeMB)], 20
    and eax, 0fffffh
    cmp eax, 0
    je  .skip
    inc dword [paddr(MemSizeMB)]
.skip:
    ret

; 打印ARDS
DisplayARDS:
    callPrintStrln MemInfoMsg
    call .printSepLine
    callPrintChar '|'
    callPrintStr MemBlockAddrMsg
    callPrintChar ' ', ADDR_MSG_GAP_LEN
    callPrintChar '|'
    callPrintStr MemBlockLengthMsg
    callPrintChar ' ', LENGTH_MSG_GAP_LEN
    callPrintChar '|'
    callPrintStr MemBlockTypeMsg
    callPrintChar '|'
    call Println
    call .printSepLine

    xor ecx, ecx
    mov cx, [paddr(ARDSNum)]
.next:
    push ecx
    xor eax, eax
    mov ax, [paddr(ARDSNum)]
    sub ax, cx
    mov bl, 20
    mul bl
    mov esi, BaseOfARDSBuffer << 4
    add esi, eax

    callPrintChar '|'
    callPrintChar '0'
    callPrintChar 'x'
    mov eax, [esi+4]    
    call PrintDigitalHex
    mov eax, [esi]
    call PrintDigitalHex
    ; 18 is the length of 64bit number hex string with prefix '0x'
    callPrintChar ' ', ADDR_MSG_GAP_LEN + MemBlockAddrMsgLen - 18
    
    callPrintChar '|'
    callPrintChar '0'
    callPrintChar 'x'
    mov eax, [esi+12]
    call PrintDigitalHex
    mov eax, [esi+8]
    call PrintDigitalHex
    callPrintChar ' ', LENGTH_MSG_GAP_LEN + MemBlockLengthMsgLen - 18

    callPrintChar '|'
    callPrintChar '0'
    callPrintChar 'x'
    mov al, [esi+16]
    and al, 0fh
    call Digital2Char
    callPrintChar
    callPrintChar ' ', 1 ; FIXME
    callPrintChar '|'
    call Println

    pop ecx
    dec ecx
    jnz .next
    call .printSepLine
    ret

.printSepLine:
    callPrintChar '+'
    callPrintChar '-', ADDR_MSG_GAP_LEN + MemBlockAddrMsgLen
    callPrintChar '+'
    callPrintChar '-', LENGTH_MSG_GAP_LEN + MemBlockLengthMsgLen
    callPrintChar '+'
    callPrintChar '-', MemBlockTypeMsgLen
    callPrintChar '+'
    call Println
    ret

;----------------------------------------------------------------------------
; 函数名: PrintDigitalHex
;----------------------------------------------------------------------------
; 作用:
;   打印eax寄存器中的数字的16进制
;   入参: eax
PrintDigitalHex:
    push eax
    push esi
    push ebp
    mov ebp, esp
    sub esp, 4   
       
    mov [ebp-4], eax

    mov ecx, 4
.print_number:
    mov esi, ebp
    add si, cx
    sub si, 5
    mov byte al, [ss:si]
    shr al, 4
    call Digital2Char
    callPrintChar

    mov byte al, [ss:si]
    call Digital2Char
    callPrintChar
    loop .print_number
 
    add esp, 4
    pop ebp
    pop esi
    pop eax
    ret

Digital2Char:
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
; 函数名: PrintAL
;----------------------------------------------------------------------------
; 作用:
;   打印AL中的字符
;   入参: al, ecx
PrintAL:
    push ecx
.next:
    mov ah, 07h
    mov di, [paddr(CursorPosition)]
    shl di, 1
    mov [gs:di], ax
    inc word [paddr(CursorPosition)]
    call SetCursor
    loop .next
    pop ecx
    ret

SetCursor:
    push ax

    mov dx, 0x3d4
    mov al, 0eh
    out dx, al
    mov dx, 0x3d5
    mov ax, [paddr(CursorPosition)]
    mov al, ah
    out dx, al

    mov dx, 0x3d4
    mov al, 0fh
    out dx, al
    mov dx, 0x3d5
    mov ax, [paddr(CursorPosition)]
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

    mov [paddr(CursorPosition)],  bx
    ret

; ecx: string length
; esi: address of string
PrintStr:
.next:
    lodsb
    callPrintChar
    loop .next
    ret

Println:
    mov dx, [paddr(CursorPosition)]
    mov ax, dx
    mov bl, 80
    div bl
    shr ax, 8
    sub dx, ax
    add dx, 80
    mov [paddr(CursorPosition)], dx
    cmp dx, 80*25
    je  .scrollup
    call SetCursor
    ret
.scrollup:
    call ScrollUpScreen
    ret

    ret

ClearScreen:
    push es

    mov ax, gs
    mov es, ax
    xor edi, edi
    xor ax, ax
    mov ecx, 80 * 25
    rep stosw
    
    mov word [paddr(CursorPosition)], 0
    call SetCursor

    pop es
    ret

ScrollUpScreen:
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
    mov ax, 0720h
    mov ecx, 80
    rep stosw

    ; restore ds and es
    pop es
    pop ds

    sub word [paddr(CursorPosition)], 80
    call SetCursor

    pop ecx
    ret

sleep:
    push ecx
    mov ecx, 0ffffffh
.next:
    loop .next
    pop ecx
    ret
