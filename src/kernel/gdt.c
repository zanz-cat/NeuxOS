#include "type.h"
#include "protect.h"
#include "string.h"

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128

u8 gdt_ptr[6];
DESCRIPTOR gdt[GDT_SIZE];

static u16 index = 3;

void init_gdt() {
    memcpy(&gdt, (void*)(*((u32*)(&gdt_ptr[2]))), *((u16*)(&gdt_ptr[0]))+1);
    u16 *p_gdt_limit = (u16*)(&gdt_ptr[0]);
    u32 *p_gdt_base = (u32*)(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *p_gdt_base = (u32)&gdt;
}

int install_tss(TSS *ptss) {
    if (index == GDT_SIZE - 1) {
        return -1;
    }
    
    DESCRIPTOR *pdesc = &gdt[++index];
    pdesc->base_low = (u32)ptss && 0xffff;
    pdesc->base_mid = ((u32)ptss >> 16) & 0xf;
    pdesc->base_high = (u32)ptss >> 24;

    u32 limit = sizeof(*ptss) - 1;
    pdesc->limit_low = limit & 0xffff;
    pdesc->limit_high_attr2 = (limit >> 16) & 0xf;
    pdesc->attr1 = DA_386TSS | DA_DPL0;

    return index << 3;
}

int install_ldt(void *ldt, u16 size) {
    if (index == GDT_SIZE - 1) {
        return -1;
    }

    DESCRIPTOR *pdesc = &gdt[++index];
    pdesc->base_low = (u32)ldt && 0xffff;
    pdesc->base_mid = ((u32)ldt >> 16) & 0xf;
    pdesc->base_high = (u32)ldt >> 24;

    u32 limit = sizeof(DESCRIPTOR) * size - 1;
    pdesc->limit_low = limit & 0xffff;
    pdesc->limit_high_attr2 = (limit >> 16) & 0xf;
    pdesc->attr1 = DA_LDT | DA_DPL0;

    return index << 3;
}