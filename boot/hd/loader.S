%include "boot/const.S"
%include "hd.S"

org 0h
jmp LABEL_START

%include "boot/utils.S"

; GDT
LABEL_GDT:              Descriptor     0,          0,              0       ; dummy descriptor
LABEL_DESC_FLAT_C:      Descriptor     0,          0fffffh,        DA_CR|DA_32|DA_LIMIT_4K
LABEL_DESC_FLAT_RW:     Descriptor     0,          0fffffh,        DA_DRW|DA_32|DA_LIMIT_4K
LABEL_DESC_VIDEO:       Descriptor     0b8000h,    0ffffh,         DA_DRW|DA_DPL3

GdtLen equ $ - LABEL_GDT
GdtPtr dw GdtLen - 1
       dd paddr(LABEL_GDT)

; GDT Selector
SelectorFlatC equ LABEL_DESC_FLAT_C - LABEL_GDT
SelectorFlatRW equ LABEL_DESC_FLAT_RW - LABEL_GDT
SelectorVideo equ LABEL_DESC_VIDEO - LABEL_GDT + SA_RPL3

TOP_OF_STACK equ 0ffffh

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov es, ax
    ; init stack
    mov sp, TOP_OF_STACK

    ; read memory info
    call ReadMemInfo

    ; load GDT
    lgdt [GdtPtr]

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

    ; load selectors to regs
    mov ax, SelectorFlatRW
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov ax, SelectorVideo
    mov gs, ax

    ; jmp to 32bit segment, clean up pipeline and cache in 16bit mode.
    jmp dword SelectorFlatC:(paddr(LABEL_PM_START))

ReadMemInfo:
    push bp
    mov bp, sp

    mov ax, (CONFIG_SHARE_DATA_ADDR >> 4)
    mov es, ax
    mov dword [es:(CONFIG_SHARE_DATA_ADDR & 0xffff)+8], 0 ; ARDS count

    sub sp, 2
    mov word [bp-2], (CONFIG_SHARE_DATA_ADDR & 0xffff)+12 ; ARDS entry address

    mov ebx, 0
.continue:
    mov di, [bp-2]
    mov ax, 0e820h
    mov ecx, 20 ; sizeof(struct ARDS) == 20
    mov edx, 0534d4150h ; 'SMAP'
    int 15h
    jc .continue    ; cf = 1, 出错重试
    ; success
    add word [bp-2], cx
    inc word [es:(CONFIG_SHARE_DATA_ADDR & 0xffff)+8]
    ; check if the last one
    cmp ebx, 0
    jnz .continue ; not the last one

    add sp, 2
    pop bp
    ret

; function: read secotrs(LBA28)
; ax - sector low
; bx - sector high
; cl numbers of sectors to read
; es:di result buffer address
; loader_read_sector:
;     push bp
;     mov bp, sp

;     push ax
;     mov dx, ATA_PORT_COUNT
;     mov al, cl
;     out dx, al
;     pop ax
;     mov dx, ATA_PORT_SECTOR
;     out dx, al
;     mov dx, ATA_PORT_CYLINDER_LOW
;     shr ax, 8
;     out dx, al
;     mov dx, ATA_PORT_CYLINDER_HIGH
;     mov ax, bx
;     out dx, al
;     mov dx, ATA_PORT_DISK_HEAD
;     shr ax, 8
;     or al, 0xe0 ; 1110 LBA mode and master disk
;     ; request read
;     mov dx, ATA_PORT_CMD_STATUS
;     mov al, ATA_CMD_READ
;     out dx, al
; .wait:
;     in al, dx ; al: [BSY - - - DRQ - - ERR] DRQ: data ready
;     and al, 88h
;     cmp al, 08h ; not BSY and DRQ
;     jne .wait

;     mov dx, ATA_PORT_DATA
;     push cx ; save cx
;     shl cx, 8 ; x 256 words (512 bytes per sector)
; .read:
;     in ax, dx
;     mov [di], ax
;     add di, 2
;     loop .read
;     pop cx

;     leave
;     ret

[BITS 32]
LABEL_PM_START: