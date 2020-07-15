%define arg(i) [ebp+4*(i+2)]

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