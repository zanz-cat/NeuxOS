SELECTOR_KERNEL_CS      equ         1000b
SELECTOR_KERNEL_DS      equ         10000b
LDT_SIZE                equ         128

OFFSET_PROC_GS          equ         0
OFFSET_PROC_FS          equ         OFFSET_PROC_GS + 4
OFFSET_PROC_ES          equ         OFFSET_PROC_FS + 4
OFFSET_PROC_DS          equ         OFFSET_PROC_ES + 4
OFFSET_PROC_EDI         equ         OFFSET_PROC_DS + 4
OFFSET_PROC_ESI         equ         OFFSET_PROC_EDI + 4
OFFSET_PROC_EBP         equ         OFFSET_PROC_ESI + 4
_OFFSET_PROC_ESP        equ         OFFSET_PROC_EBP + 4
OFFSET_PROC_EBX         equ         _OFFSET_PROC_ESP + 4
OFFSET_PROC_EDX         equ         OFFSET_PROC_EBX + 4
OFFSET_PROC_ECX         equ         OFFSET_PROC_EDX + 4
OFFSET_PROC_EAX         equ         OFFSET_PROC_ECX + 4
OFFSET_PROC_EIP         equ         OFFSET_PROC_EAX + 4
OFFSET_PROC_CS          equ         OFFSET_PROC_EIP + 4
OFFSET_PROC_EFLAGS      equ         OFFSET_PROC_CS + 4
OFFSET_PROC_ESP         equ         OFFSET_PROC_EFLAGS + 4
OFFSET_PROC_SS          equ         OFFSET_PROC_ESP + 4
OFFSET_PROC_PID         equ         OFFSET_PROC_SS + 4
OFFSET_PROC_TYPE        equ         OFFSET_PROC_PID + 4
OFFSET_PROC_LDT         equ         OFFSET_PROC_TYPE + 4
OFFSET_PROC_LDT_SEL     equ         OFFSET_PROC_LDT + 8 * LDT_SIZE
OFFSET_PROC_STACK       equ         OFFSET_PROC_LDT_SEL + 4

OFFSET_TSS_PREV         equ         0
OFFSET_TSS_ESP0         equ         OFFSET_TSS_PREV + 4
