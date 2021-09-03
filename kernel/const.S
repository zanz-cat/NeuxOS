SELECTOR_KERNEL_CS      equ         1000b
SELECTOR_KERNEL_DS      equ         10000b
LDT_SIZE                equ         128
STACK0_SIZE             equ         10240
STACK3_SIZE             equ         10240

OFFSET_PROC_PID         equ         0
OFFSET_PROC_TYPE        equ         OFFSET_PROC_PID + 4
OFFSET_PROC_STATE       equ         OFFSET_PROC_TYPE + 1
OFFSET_PROC_SS0         equ         OFFSET_PROC_STATE + 1
OFFSET_PROC_ESP0        equ         OFFSET_PROC_SS0 + 2
OFFSET_PROC_TICKS       equ         OFFSET_PROC_ESP0 + 4
OFFSET_PROC_TTY         equ         OFFSET_PROC_TICKS + 4
OFFSET_PROC_LDT_SEL     equ         OFFSET_PROC_TTY + 4
OFFSET_PROC_LDT         equ         OFFSET_PROC_LDT_SEL + 2
OFFSET_PROC_STACK0_TOP  equ         OFFSET_PROC_LDT + 8 * LDT_SIZE
OFFSET_PROC_STACK0      equ         OFFSET_PROC_STACK0_TOP + STACK0_SIZE
OFFSET_PROC_STACK3_TOP  equ         OFFSET_PROC_STACK0
OFFSET_PROC_STACK3      equ         OFFSET_PROC_STACK3_TOP + STACK3_SIZE

OFFSET_TSS_PREV         equ         0
OFFSET_TSS_ESP0         equ         OFFSET_TSS_PREV + 4
OFFSET_TSS_SS0          equ         OFFSET_TSS_ESP0 + 4

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