LoaderBase          equ     07000h
LoaderOffset        equ     0100h

%define paddr(o)   (LoaderBase << 4) + o