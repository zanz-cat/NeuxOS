BaseOfLoader          equ     09000h
OffsetOfLoader        equ     0100h

%define paddr(o)   (BaseOfLoader << 4) + o