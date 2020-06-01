SELECTOR_KERNEL_CS  equ 1000b

global _start

extern gdt_ptr
extern idt_ptr
extern init_gdt
extern kernel_idle

extern init_interrupt
extern clear_screen

extern init_system

extern current

[SECTION .bss]
stack_space  resb    2 * 1024
stacktop: 

[SECTION .text]

_start:
    ; init kernel stack
    mov esp, stacktop

    ; move GDT to kernel
    sgdt [gdt_ptr]
    call init_gdt
    lgdt [gdt_ptr]

    ; init interrupt
    call init_interrupt
    lidt [idt_ptr]

    ; jmp with new GDT, make sure GDT correct
    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    ; init system
    call init_system

    ; enable interrupt
    ; sti

    mov eax, [current]
    lldt word [eax+1040]
    ltr word [eax+1148]
    push dword [eax+8]
    push dword [eax+12]
    mov ecx, 15
    lea esi, [eax+3200-4]
    lea edi, [esp-4]
    std
    rep movsd
    cld
    sub esp, 15*4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd

    ; main loop
.hlt:
    hlt
    jmp .hlt
