BaseOfLoader          equ     09000h
OffsetOfLoader        equ     0100h

BaseOfLoaderPhyAddr   equ     BaseOfLoader << 4

%define paddr(s)   BaseOfLoaderPhyAddr + s