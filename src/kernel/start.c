#include "const.h"
#include "string.h"
#include "type.h"

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128

PUBLIC u8  gdt_ptr[6];
PUBLIC DESCRIPTOR  gdt[GDT_SIZE];

PUBLIC void cstart(){
    memcpy(&gdt, (void*)(*((u32*)(&gdt_ptr[2]))), *((u16*)(&gdt_ptr[0]))+1);
    u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
    u32* p_gdt_base = (u32*)(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *p_gdt_base = (u32)&gdt;
}
