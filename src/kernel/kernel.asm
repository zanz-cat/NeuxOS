SELECTOR_KERNEL_CS  equ 1000b

extern gdt_ptr
extern idt_ptr
extern relocate_gdt
extern init_interrupt
extern display_banner
extern sleep
extern clear_screen

extern create_proc
extern current

extern app1
extern app2

[SECTION .bss]
StackSpace  resb    2 * 1024
StackTop: 

[SECTION .text]
global _start

_start:
    ; init kernel stack
    mov esp, StackTop

    ; move GDT to kernel
    sgdt [gdt_ptr]
    call relocate_gdt
    lgdt [gdt_ptr]

    ; init interrupt
    call init_interrupt
    lidt [idt_ptr]

    ; jmp with new GDT, make sure GDT correct
    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    ; display banner
    call display_banner

    jmp SELECTOR_KERNEL_CS:schedule

schedule:
    ; create proc
    push _proc1
    call create_proc
    add esp, 4

    push _proc2
    call create_proc
    add esp, 4

    ; clean screen
    call clear_screen
    
    ; enable interrupt
    sti

    ; main loop
.hlt:
    hlt
    jmp .hlt

_proc1:
    call app1
    ret

_proc2:
    call app2
    ret