%include "kernel.inc"

org 0100h
    ;xchg bx, bx
    jmp LABEL_BEGIN

[SECTION .gdt]
;                                Base,         Limit,            Attributes                      
LABEL_GDT:            Descriptor 0,            0,                0
LABEL_DESC_NORMAL:    Descriptor 0,            0ffffh,           DA_DRW
LABEL_DESC_CODE32:    Descriptor 0,            SegCode32Len - 1, DA_C + DA_32
LABEL_DESC_CODE16:    Descriptor 0,            0ffffh,           DA_C
LABEL_DESC_DATA:      Descriptor 0,            DataLen - 1,      DA_DRW
LABEL_DESC_STACK:     Descriptor 0,            TopOfStack - 1,   DA_DRWA+DA_32
LABEL_DESC_TEST:      Descriptor 0500000h,     0ffffh,           DA_DRW
LABEL_DESC_VIDEO:     Descriptor 0b8000h,      0ffffh,           DA_DRW
LABEL_DESC_LDT1:      Descriptor 0,            LDT1Len - 1,      DA_LDT

GDTLen equ $ - LABEL_GDT
GDTPtr dw GDTLen - 1	;GDT Limit
       dd 0		;GDT Base Address

;GDT Selectors
SelectorNormal        equ LABEL_DESC_NORMAL - LABEL_GDT
SelectorCode32        equ LABEL_DESC_CODE32 - LABEL_GDT
SelectorCode16        equ LABEL_DESC_CODE16 - LABEL_GDT
SelectorData          equ LABEL_DESC_DATA   - LABEL_GDT
SelectorStack         equ LABEL_DESC_STACK  - LABEL_GDT
SelectorTest          equ LABEL_DESC_TEST   - LABEL_GDT
SelectorVideo         equ LABEL_DESC_VIDEO  - LABEL_GDT
SelectorLDT1          equ LABEL_DESC_LDT1   - LABEL_GDT

[SECTION .ldt1]
ALIGN 32
LABEL_LDT1:
LABEL_DESC_TASK1:	Descriptor 	0,	SegCodeTask1Len - 1,	DA_C + DA_32

LDT1Len	equ $ - LABEL_LDT1

SelectorTask1	equ LABEL_DESC_TASK1 - LABEL_LDT1 + SA_TIL

[SECTION .data1]
ALIGN 32
[BITS 32]
LABEL_DATA:
SPValueInRealMode	dw 0

PMMessage:	db "In Protest Mode now. @_@", 0
OffsetPMMessage	equ PMMessage - $$

StrTest:	db "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0
OffsetStrTest	equ StrTest - $$

DataLen		equ $ - LABEL_DATA


[SECTION .gs]
ALIGN 32
[BITS 32]
LABEL_STACK: times 512 db 0
TopOfStack equ $ - LABEL_STACK 


[SECTION .s16]
[BITS 16]
LABEL_BEGIN:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0100h

	mov [LABEL_GO_BACK_TO_REAL + 3], ax
	mov [SPValueInRealMode], sp

	; initialize 16bit code segment descriptor
	mov ax, cs
	movzx eax, ax
	shl eax, 4
	add eax, LABEL_SEG_CODE16
	mov word [LABEL_DESC_CODE16 + 2], ax
	shr eax, 16
	mov byte [LABEL_DESC_CODE16 + 4], al
	mov byte [LABEL_DESC_CODE16 + 7], ah

    ; initialize 32bit code segment descriptor
    xor eax, eax
    mov ax, cs
    shl eax, 4
    add eax, LABEL_SEG_CODE32
    mov word [LABEL_DESC_CODE32 + 2], ax
    shr eax, 16
    mov byte [LABEL_DESC_CODE32 + 4], al
    mov byte [LABEL_DESC_CODE32 + 7], ah

	; initialize data segment descriptor
	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, LABEL_DATA
	mov word [LABEL_DESC_DATA + 2], ax
	shr eax, 16
	mov byte [LABEL_DESC_DATA + 4], al
	mov byte [LABEL_DESC_DATA + 7], ah

	; initialize stack segment descriptor
	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, LABEL_STACK
	mov word [LABEL_DESC_STACK + 2], ax
	shr eax, 16
	mov byte [LABEL_DESC_STACK + 4], al
	mov byte [LABEL_DESC_STACK + 7], ah

	; initialize ldt1 segment descriptor
	xor eax, eax
	mov ax, ds
	shl eax, 4
	add eax, LABEL_LDT1
	mov word [LABEL_DESC_LDT1 + 2], ax
	shr eax, 16
	mov byte [LABEL_DESC_LDT1 + 4], al
	mov byte [LABEL_DESC_LDT1 + 7], ah


	; initialize task1 segment descriptor
	xor eax, eax
	mov ax, cs
	shl eax, 4
	add eax, LABEL_SEG_TASK1
	mov word [LABEL_DESC_TASK1 + 2], ax
	shr eax, 16
	mov byte [LABEL_DESC_TASK1 + 4], al
	mov byte [LABEL_DESC_TASK1 + 7], ah

    ; initialize GDT Pointer
    xor eax, eax
    mov ax, ds
    shl eax, 4
    add eax, LABEL_GDT
    mov dword [GDTPtr + 2], eax

    lgdt [GDTPtr]

    cli

    in al, 92h
    or al, 00000010b
    out 92h, al

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp dword SelectorCode32:0


LABEL_REAL_ENTRY:
	xchg bx, bx
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax

	mov sp, [SPValueInRealMode]
	in al, 92h
	and al, 11111101b
	out 92h, al

	sti

	mov ax, 4c00h
	int 21h


[SECTION .s32]
[BITS 32]
LABEL_SEG_CODE32:
	mov ax, SelectorData
	mov ds, ax
	mov ax, SelectorTest
	mov es, ax
    mov ax, SelectorVideo
    mov gs, ax
	mov ax, SelectorStack
	mov ss, ax

	mov esp, TopOfStack

	; display a string
	mov ah, 0ch
	xor esi, esi
	xor edi, edi
	mov esi, OffsetPMMessage
	mov edi, (80 * 10 +0 ) * 2
	cld
.1:
	lodsb
	test al, al
	jz	.2
	mov [gs:edi], ax
	add edi, 2
	jmp .1
.2:
	
	call DispReturn
	call TestRead
	call TestWrite
	call TestRead

	mov ax, SelectorLDT1
	lldt ax
	jmp SelectorTask1:0

TestRead:
	xor esi, esi
	mov ecx, 8
.loop:
	mov al, [es:esi]
	call DispAL
	inc esi
	loop .loop

	call DispReturn
	ret


TestWrite:
	push esi
	push edi
	xor esi, esi
	xor edi, edi
	mov esi, OffsetStrTest
	cld
.1:
	lodsb
	test al, al
	jz .2
	mov [es:edi], al
	inc edi
	jmp .1
.2:

	pop edi
	pop esi

	ret


; Display digital in AL register
; the modified registers: ax, edi
DispAL:
	push ecx
	push edx
	
	mov ah, 0ch
	mov dl, al
	shr al, 4
	mov ecx, 2
.begin:
	and al, 0fh
	cmp al, 9
	ja .1
	add al, '0'
	jmp .2
.1:
	sub al, 0ah
	add al, 'A'
.2:
	
	mov [gs:edi], ax
	add edi, 2

	mov al, dl
	loop .begin

	; edi always points to the next position
	add edi, 2

	pop edx
	pop ecx

	ret

DispReturn:
	push eax
	push ebx
	mov eax, edi
	mov bl, 160
	div bl
	and eax, 0ffh
	inc eax
	mov bl, 160
	mul bl
	mov edi, eax
	pop ebx
	pop eax

	ret

SegCode32Len equ $ - LABEL_SEG_CODE32


[SECTION .s16code]
ALIGN 32
[BITS 16]
LABEL_SEG_CODE16:
	xchg bx, bx
	; jump to real mode
	mov ax, SelectorNormal
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	mov eax, cr0
	and al, 11111110b
	mov cr0, eax

LABEL_GO_BACK_TO_REAL:
	jmp 0:LABEL_REAL_ENTRY

Code16Len	equ $ - LABEL_SEG_CODE16

[SECTION .task1]
ALIGN 32
[BITS 32]
LABEL_SEG_TASK1:
	nop
	jmp SelectorCode16:0

SegCodeTask1Len	equ $ - LABEL_SEG_TASK1
