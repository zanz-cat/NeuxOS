org 07c00h

jmp short LABEL_BEGIN
nop

BS_OEMName			db	'Lee Link'
BPB_BytesPerSec		dw	512
BPB_SecPerClus		db	1
BPB_RsvdSecCnt		dw 	1
BPB_NumFATs			db	2
BPB_RootEntCnt		dw	224
BPB_TotSec			dw	2880
BPB_Media			db	0xf0
BPB_FATSz16			dw	9
BPB_SecPerTrk		dw	18
BPB_NumHeads		dw	2
BPB_HiddSec			dd	0
BPB_TotSec32		dd	0
BS_DrvNum			db	0
BS_Reserved1		db	0
BS_BootSig			db	29h
BS_VolID			dd	0
BS_VolLab			db	'LEELINKOS01'
BS_FileSysType		db	'FAT12   '

BaseOfLoader		equ 09000h
OffsetOfLoader		equ 0100h

LABEL_BEGIN:
	; init stack
	mov ax, cs
	mov es, ax
	mov ds, ax
	mov ss, ax
	mov ss, ax
	mov sp, TopOfStack

	; clear screen
	mov ah, 06h
	mov al, 0h
	mov bh, 07h
	mov ch, 0
	mov cl, 0
	mov dh, 24
	mov dl, 79
	int 10h

	; display boot message
	mov ax, cs
	mov es, ax
	mov ax, BootMessage
	mov bp, ax
	mov ah, 13h
	mov al, 01h
	mov bh, 0h
	mov bl, 07h
	mov cx, BootMessageLen
	xor	dx, dx
	int 10h

	; reset floppy driver
	xor ah, ah
	xor dl, dl
	int 13h

	; read root directory
	mov ax, BaseOfLoader
	mov es, ax
	mov bx, OffsetOfLoader
	mov ax, 19
	mov cl, 14 ; BPB_RootEntCnt * 32 / BPB_BytesPerSec	
	call ReadSector

	; read loader
	xchg bx, bx
	call SearchAndReadLoader

	jmp $


; function: read secotr
; ax sector, start from 0
; cl numbers of sectors to read
; es:bx result buffer address
ReadSector:
	push bp
	mov bp, sp
	sub sp, 1

	mov dl, 18
	div dl
	inc ah
	push ax
	and ax, 0ffh
	mov dl, 2
	div dl

	mov dl, 0
	mov ch, al
	mov dh, ah
	pop ax
	shr ax, 8
	mov [bp-1], al
	mov al, cl
	mov cl, [bp-1]
	mov ah, 02h
.GoOnReading:
	int 13h
	jc	.GoOnReading

	add sp, 1
	pop bp
	ret

; function: search file `LOADER.BIN`, 
; and copy it from floppy to memory.
SearchAndReadLoader:
	cld	
	mov si, LoaderName
	mov ax, BaseOfLoader
	mov es, ax
	mov di, OffsetOfLoader

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
	jz .ret
	add di, 32
	loop .a1

	; print 'Loader not found.'
    mov ax, cs
    mov es, ax
    mov ax, LoaderNotFound
    mov bp, ax
    mov ah, 13h
    mov al, 01h
    mov bh, 0h
    mov bl, 07h
    mov cx, LoaderNotFoundLen
    xor dx, dx
    int 10h
	
.ret:	
	ret

BootMessage		db 'Booting...'
BootMessageLen  equ $ - BootMessage

LoaderNotFound db 'Loader not found.'
LoaderNotFoundLen	equ $ - LoaderNotFound

LoaderName		db	'LOADER  BIN'
LoaderNameLen	equ	$ - LoaderName

Stack times 128 db 0
StackLen equ $ - Stack
TopOfStack equ Stack + StackLen + 1

times 510 - ($-$$) db 0
dw 0xaa55
