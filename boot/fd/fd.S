CONST_BPB_BytesPerSec       equ      512
CONST_BPB_SecPerClus        equ      1
CONST_BPB_RsvdSecCnt        equ      1
CONST_BPB_NumFATs           equ      2
CONST_BPB_RootEntCnt        equ      224
CONST_BPB_TotSec            equ      2880
CONST_BPB_Media             equ      0xf0
CONST_BPB_FATSz16           equ      9
CONST_BPB_SecPerTrk         equ      18
CONST_BPB_NumHeads          equ      2
CONST_BPB_HiddSec           equ      0
CONST_BPB_TotSec32          equ      0
CONST_BS_DrvNum             equ      0
CONST_BS_Reserved1          equ      0
CONST_BS_BootSig            equ      29h
CONST_BS_VolID              equ      0

CONST_SectorOfRootDir       equ      19
CONST_FAT12_RootEntSize     equ      32
CONST_FAT12_BitsPerClus     equ      12

FAT_BASE                 equ 0000h
FAT_OFFSET               equ 7e00h

KERNEL_FILE_BASE          equ 7000h
KERNEL_FILE_OFFSET        equ 0000h

PAGE_DIR_ADDR      equ 100000h
PAGE_TABLE_ADDR    equ 101000h