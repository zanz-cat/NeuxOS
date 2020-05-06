#include "const.h"
#include "string.h"
#include "type.h"

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128
#define IDT_SIZE    256

PUBLIC u8 gdt_ptr[6];
PUBLIC DESCRIPTOR gdt[GDT_SIZE];

PUBLIC u8 idt_ptr[6];
PUBLIC GATE idt[IDT_SIZE];

PUBLIC void cstart(){
    memcpy(&gdt, (void*)(*((u32*)(&gdt_ptr[2]))), *((u16*)(&gdt_ptr[0]))+1);
    u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
    u32* p_gdt_base = (u32*)(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *p_gdt_base = (u32)&gdt;

    u16* p_idt_limit = (u16*)idt_ptr;
    u32* p_idt_base = (u32*)(idt_ptr+2);
    *p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
    *p_idt_base = (u32)&idt;
}

PUBLIC void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags) {
    int i;    
}