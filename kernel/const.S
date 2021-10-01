SELECTOR_KERNEL_CS      equ         1000b
SELECTOR_KERNEL_DS      equ         10000b
LDT_SIZE                equ         16
STACK0_SIZE             equ         10240
STACK3_SIZE             equ         10240

OFFSET_TASK_PID         equ         0
OFFSET_TASK_TSS_SEL     equ         OFFSET_TASK_PID + 4
OFFSET_TASK_STATE       equ         OFFSET_TASK_TSS_SEL + 2

INT_M_CTL               equ         0x20
INT_M_CTLMASK           equ         0x21
INT_S_CTL               equ         0xa0
INT_S_CTLMASK           equ         0xa1
INT_EOI                 equ         0x20

%macro SAVE_STATE 0
    pusha
    push ds
    push es
    push fs
    push gs
%endmacro

%macro RESTORE_STATE 0
    pop gs
    pop fs
    pop es
    pop ds
    popa
%endmacro