%include "kernel.inc"

org 0100h
    xchg bx, bx
    jmp LABEL_BEGIN

[SECTION .gdt]
;                                Base,         Limit,            Attributes                      
LABEL_GDT:            Descriptor 0,            0,                0
LABEL_DESC_NORMAL:    Descriptor 0,            0ffffh,           DA_DRW
LABEL_DESC_CODE32:    Descriptor 0,            SegCode32Len - 1, DA_C + DA_32
LABEL_DESC_CODE16:    Descriptor 0,            0ffffh,           DA_C
LABEL_DESC_DATA:      Descriptor 0,            DataLen - 1,      DA_DRW
LABEL_DESC_STACK:     Descriptor 0,            TopOfStack,       DA_DRWA+DA_32
LABEL_DESC_TEST:      Descriptor 0500000h,     0ffffh,           DA_DRW
LABEL_DESC_VIDEO:     Descriptor 0b8000h,      0ffffh,           DA_DRW

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
TopOfStack equ $ - LABEL_STACK - 1


[SECTION .s16]
[BITS 16]
LABEL_BEGIN:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov es, ax
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
	add eax, LABLE_STACK
	mov word [LABEL_DESC_STACK + 2], ax
	shr eax, 16
	mov byte [LABEL_DESC_STACK + 4], al
	mov byte [LABEL_DESC_STACK + 7], ah

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

[SECTION .s32]
[BITS 32]
LABEL_SEG_CODE32:
    mov ax, SelectorVideo
    mov gs, ax

    mov edi, (80 * 11 + 79) * 2
    mov ah, 0ch
    mov al, 'P'
    mov [gs:edi], ax

    jmp $

SegCode32Len equ $ - LABEL_SEG_CODE32
