%include "hd.S"

org MBR_OF_ADDR

TOP_OF_STACK equ 0ffffh

; struct partition {
;     uint8_t bootable; /* 0x80 means active */
;     uint8_t s_head;
;     uint16_t s_sector_cylinder; /* 0~5 sector,  6~15 cylinder */
;     uint8_t type;
;     uint8_t e_head;
;     uint16_t e_sector_cylinder;
;     uint32_t lba;
;     uint32_t count;
; };

; read mbr, load first boot
LABEL_START:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    ; init stack
    mov sp, TOP_OF_STACK

    mov cx, MBR_PART_COUNT
.a1:
    mov byte [BootPartition], MBR_PART_COUNT
    sub byte [BootPartition], cl    ; BootPartition is index
    mov al, MBR_PART_DESC_SIZE
    mul byte [BootPartition]
    mov si, DPT_OF_ADDR
    add si, ax
    cmp byte [si+PART_DESC_BOOT_OFFSET], 0x80
    je .a2
    loop .a1
    mov bp, NoBootablePartMsg
    mov cx, NoBootablePartMsgLen
    call DispStr
    hlt
.a2:
    cmp byte [si+PART_DESC_TYPE_OFFSET], FS_TYPE_EXT2
    jne .not_sup_fs
    mov ah, 02h ; read sector
    mov al, EXT2_SECTS_PER_BLK ; sector count to read
    mov cx, [si+PART_DESC_SECT_CYLIND_OFFSET]
    mov dh, [si+PART_DESC_HEAD_OFFSET]
    mov dl, 80h ; hd driver 0
    mov bx, OBR_OF_ADDR
    int 13h
    jc .error
    add byte [BootPartition], '1' - 0
    mov bp, BootMsg
    mov cx, BootMsgLen
    call DispStr
    jmp OBR_OF_ADDR ; si is bootable partition info in MBR
.error:
    mov bp, ReadSecErrMsg
    mov cx, ReadSecErrMsgLen
    call DispStr
    hlt
.not_sup_fs:
    mov bp, UnsupportFSMsg
    mov cx, UnsupportFSMsgLen
    call DispStr
    hlt

; Display string at cursor position
; es:bp String address offset
; cx String length
DispStr:
    push si
    push cx

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
 
    pop cx
    pop si
    ret

BootMsg db 'Use partiton '
BootPartition db 0
db ', ',
BootMsgLen equ $ - BootMsg

NoBootablePartMsg db 'No bootable partiton!'
NoBootablePartMsgLen equ $ - NoBootablePartMsg

ReadSecErrMsg db 'Read sector error'
ReadSecErrMsgLen equ   $ - ReadSecErrMsg

UnsupportFSMsg db 'Not supported FS'
UnsupportFSMsgLen equ   $ - UnsupportFSMsg