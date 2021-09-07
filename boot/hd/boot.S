%include "boot/const.S"
%include "hd.S"

org OBR_OF_ADDR
jmp LABEL_START

%include "boot/utils.S"

TOP_OF_STACK equ 0ffffh
BlockGroupPtr dw 0
PartStartLBA dd 0

LABEL_START:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, TOP_OF_STACK

    mov bp, BootMsg
    mov cx, BootMsgLen
    call DispStr

    mov eax, [si+PART_DESC_LBA_OFFSET] ; si set by mbr
    mov [PartStartLBA], eax

    ; read superblock
    mov bx, SUPBLK_OF_ADDR
    mov eax, 1
    call GetBlockAbsLBA
    mov cl, 1
    call ReadSector
    ; compute group desc sector count to read
    mov eax, [SUPBLK_OF_ADDR+EXT2_S_OF_BLOCKS_COUNT]
    toDAX
    mov ecx, [SUPBLK_OF_ADDR+EXT2_S_OF_BLOCKS_PER_GROUP]
    call divdw
    toEAX
    cmp cx, 0
    je .a1
    inc eax
.a1:
    ; eax = BG count
    test eax, 0xffffff00
    jnz TooLarge
    push ax ; save BG count
    mov bx, EXT2_BG_DESC_SIZE
    mul bx
    mov cx, CONFIG_EXT2_BS
    call divdw
    toEAX
    cmp cx, 0
    je .a2
    inc eax
.a2:
    ; eax = blocks count of BG descriptors
    test eax, 0xff00
    jnz TooLarge
    mov bx, EXT2_SECTS_PER_BLK
    mul bx
    cmp dx, 0
    jne TooLarge
    ; ax = sectors count of BG descriptors
    cmp ah, 0
    jne TooLarge
    mov cl, al
    mov eax, 2; skip boot and super blocks
    call GetBlockAbsLBA
    mov bx, BG_DESC_OF_ADDR
    call ReadSector ; read bg descriptors
    mov bx, sp
    mov cx, [ss:bx] ; BG count
.search_bg:
    mov bx, sp
    mov di, [ss:bx]
    sub di, cx ; di is bg index
    mov ax, EXT2_BG_DESC_SIZE
    mul di
    mov di, BG_DESC_OF_ADDR
    add di, ax ; di -> some BG desc
    mov ax, EXT2_BG_DESC_SIZE
    mul word [ss:bx]
    mov bx, ax
    add bx, BG_DESC_OF_ADDR ; bx -> the end of bg descriptors
    push cx
    mov eax, [di+EXT2_BG_OF_INO_BITMAP]
    call GetBlockAbsLBA
    mov cl, EXT2_SECTS_PER_BLK
    call ReadSector ; read inode bitmap
    mov ecx, EXT2_INO_ROOT
    sub ecx, [SUPBLK_OF_ADDR+EXT2_S_OF_FIRST_DATA_BLOCK]
    mov al, 1
    shl al, cl
    test byte [bx], al ; match root inode bit
    jz .continue
    mov [BlockGroupPtr], di
    jmp SearchInRoot
.continue:
    pop cx
    loop .search_bg
    jmp NotFound

SearchInRoot:
    mov eax, [di+EXT2_BG_OF_INO_TABLE]
    call GetBlockAbsLBA
    push eax ; inode table sector no.
    mov eax, EXT2_INO_ROOT+1 ; only read root inode
    mov cx, [SUPBLK_OF_ADDR+EXT2_S_OF_INO_SIZE]
    toEAX
    call muldw
    mov cx, CONFIG_HD_SECT_SZ
    call divdw
    toEAX
    cmp cx, 0
    je .a1
    inc eax
.a1:
    test eax, 0xffffff00 ; al is sector count to read
    jnz TooLarge
    mov cl, al
    pop eax
    call ReadSector ; read inode table
    ; [bx] store inode table now
    mov ax, [SUPBLK_OF_ADDR+EXT2_S_OF_INO_SIZE]
    mov ecx, EXT2_INO_ROOT
    sub ecx, [SUPBLK_OF_ADDR+EXT2_S_OF_FIRST_DATA_BLOCK]
    test ecx, 0xffff0000
    jnz TooLarge
    mul cx
    test dx, 0xffff
    jnz TooLarge
    mov di, bx
    add di, ax ; di -> root inode
    mov eax, [di+EXT2_I_OF_BLOCKS]
    call GetBlockAbsLBA
    mov cl, EXT2_SECTS_PER_BLK*12 ; only scan direct block pointer, i.e. the first 12 blocks
    call ReadSector ; [bx] store root dir entries now
    jmp ScanRootDir
ScanRootDir:
    cld
    mov di, 0
.next:
    cmp dword [bx+di+EXT2_DIR_REC_OF_INO], 0
    je .next_post ; 0 indicate the entry is not used
    cmp word [bx+di+EXT2_DIR_REC_OF_LEN], 0
    je NotFound
    cmp byte [bx+di+EXT2_DIR_REC_OF_NAME_LEN], LoaderNameLen
    je .cmp_name
    cmp byte [bx+di+EXT2_DIR_REC_OF_TYPE], EXT_FT_REG_FILE
    je .cmp_name
    jmp .next_post
.cmp_name:
    push si
    push di
    mov cx, LoaderNameLen
    lea si, [bx+di+EXT2_DIR_REC_OF_NAME]
    mov di, LoaderName
    repe cmpsb
    pop di
    pop si
    jz ReadLoader ; di = loader.bin offset in dir
.next_post:
    mov ax, di
    add ax, [bx+di+EXT2_DIR_REC_OF_LEN]
    mov di, ax
    jmp .next

ReadLoader:
    ; [bx+di] store loader.bin dir entry
    mov eax, [bx+di]
    dec eax ; inode no. start from 1, 0 indicate the entry not used.
    toDAX
    mov cx, [SUPBLK_OF_ADDR+EXT2_S_OF_INO_SIZE]
    call muldw
    mov cx, CONFIG_EXT2_BS
    call divdw
    toEAX
    mov di, [BlockGroupPtr]
    add eax, [di+EXT2_BG_OF_INO_TABLE]
    mov di, cx ; inode offset in block
    call GetBlockAbsLBA
    mov cl, EXT2_SECTS_PER_BLK
    call ReadSector
    ; bx+di -> loader.bin's inode
    mov eax, [bx+di+EXT2_I_OF_SIZE]
    toDAX
    mov cx, CONFIG_EXT2_BS
    call divdw
    toEAX
    cmp cx, 0
    je .a1
    inc eax
.a1:
    mov ecx, eax ; data block count to read
    jmp ReadLoaderData

; max data blocks: 268
ReadLoaderData:
    push bp
    mov bp, sp

    push ecx ; [bp-4]
    push word LOADER_OFFSET ; [bp-6] store dest address offset
    push word 0 ; [bp-8] loop count
    push bx ; [bp-10] memory address following BG descriptors

    lea bx, [bx+di] ; bx -> loader.bin's inode
    mov ax, LOADER_BASE
    mov es, ax
    mov cx, 12 ; direct pointer
    lea si, [bx+EXT2_I_OF_BLOCKS]
    call ReadLoaderBlocks
    cmp dword [bp-4], 0
    je .read_fin
    ; read Level 1 indirect block
    lea si, [bx+EXT2_I_OF_BLOCKS+EXT2_I_BLOCKS_L1_INDEX*4]
    mov eax, [si]
    call GetBlockAbsLBA
    push es
    push bx
    mov bx, ds
    mov es, bx
    mov bx, [bp-10] ; will override loader.bin's inode
    mov cl, EXT2_SECTS_PER_BLK
    call ReadSector
    pop bx
    pop es
    mov cx, CONFIG_EXT2_BS/4
    mov si, [bp-10]
    call ReadLoaderBlocks
    cmp dword [bp-4], 0
    je .read_fin
    jmp LoaderTooLarge
.read_fin:
    jmp LOADER_BASE:LOADER_OFFSET

; si - blocks addr, cx - blocks count
; use ReadLoaderData stack
ReadLoaderBlocks:
    push bx
    mov [bp-8], cx
.loop_body:
    mov di, [bp-8]
    sub di, cx ; di is index
    mov ax, di
    mov dl, 4 ; sizeof(block pointer)
    mul dl
    mov bx, ax
    mov eax, [si+bx]
    call GetBlockAbsLBA
    push cx
    mov bx, [bp-6]
    mov cl, EXT2_SECTS_PER_BLK
    call ReadSector
    pop cx
    add word [bp-6], CONFIG_EXT2_BS
    dec dword [bp-4]
    cmp dword [bp-4], 0
    je .loop_end
    loop .loop_body
.loop_end:
    pop bx
    ret

; input: eax, block no
; output: eax, absolute LBA
GetBlockAbsLBA:
    push cx
    mov cx, EXT2_SECTS_PER_BLK
    toDAX
    call muldw
    toEAX
    add eax, [PartStartLBA]
    pop cx
    ret

TooLarge:
    mov bp, PartitionTooLarge
    mov cx, PartitionTooLargeLen
    call DispStr
    hlt

LoaderTooLarge:
    mov bp, LoaderTooLargeStr
    mov cx, LoaderTooLargeStrLen
    call DispStr
    hlt

NotFound:
    mov bp, LoaderNotFoundMsg
    mov cx, LoaderNotFoundMsgLen
    call DispStr
    hlt

; function: read secotrs
; eax(0 ~ 23bit valid) start sector, start from 0
; cl numbers of sectors to read
; es:bx result buffer address
ReadSector:
    push bp
    mov bp, sp

    push eax
    push bx
    mov edx, eax
    shr edx, 16
    mov bx, CONFIG_HD_SPT
    div bx
    inc dx  ; dl is sector
    cmp dh, 0
    jne .error
    ; ax is track count
    mov bl, CONFIG_HD_HEADS
    div bl
    pop bx

    mov ch, al  ; cylinder
    mov dh, ah  ; head
    mov al, cl  ; count
    mov cl, dl  ; sector
    mov ah, 02h
    mov dl, 80h ; harddisk
    int 13h
    jc .error
    pop eax
    leave
    ret
.error:
    mov bp, ReadSectorErr
    mov cx, ReadSectorErrLen
    call DispStr
    hlt

; display string at cursor position
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

BootMsg db 'booting loader', 0dh, 0ah
BootMsgLen equ $ - BootMsg

LoaderNotFoundMsg db 'Loader not found!'
LoaderNotFoundMsgLen equ $ - LoaderNotFoundMsg

PartitionTooLarge db 'Error! partition too large'
PartitionTooLargeLen equ $ - PartitionTooLarge

LoaderTooLargeStr db 'Loader too large!'
LoaderTooLargeStrLen equ $ - LoaderTooLargeStr

ReadSectorErr db 'Read sector error!'
ReadSectorErrLen equ $ - ReadSectorErr

LoaderName            db    'loader.bin'
LoaderNameLen         equ   $ - LoaderName