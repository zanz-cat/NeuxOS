#include <type.h>
#include <kernel/protect.h>
#include <lib/string.h>
#include <lib/log.h>

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128

struct gdtr {
    u16 limit;
    void *base;
} __attribute__((packed));

static struct descriptor gdt[GDT_SIZE];
static u8 bitmap[GDT_SIZE/8];

#define BITMAP_SET(index) bitmap[index/8] |= (1 << (index%8))
#define BITMAP_CLR(index) bitmap[index/8] &= (~(1 << (index%8)))
#define BITMAP_GET(index) (bitmap[index/8] & (1 << (index%8)))

static int alloc() 
{
    int i;
    for (i = 0; i < GDT_SIZE && BITMAP_GET(i); i++);
    if (i > GDT_SIZE - 1) {
        log_error("no gdt space left\n");
        return -1;
    }

    BITMAP_SET(i);
    return i;
}

static void free(int index) 
{
    if (index > GDT_SIZE - 1)
        return;

    BITMAP_CLR(index);
}

static int uninstall_desc(u16 sel) 
{
    int index = sel >> 3;
    if (index > GDT_SIZE - 1)
        return -1;

    memset(&gdt[index], 0, sizeof(gdt[index]));
    free(index);

    return 0;
}

void gdt_init() 
{
    struct gdtr gdtr;

    asm("sgdt %0":"=m"(gdtr)::);
    memcpy(&gdt, gdtr.base, gdtr.limit + 1);
    gdtr.base = &gdt;
    gdtr.limit = GDT_SIZE * sizeof(struct descriptor) - 1;
    asm("lgdt %0"::"m"(gdtr):);

    BITMAP_SET(0);
    BITMAP_SET(1);
    BITMAP_SET(2);
    BITMAP_SET(3);
}

// TODO: can i install tss into ldt?
int install_tss(struct tss *ptss) 
{
    int index = alloc();
    if (index < 0) {
        log_error("alloc gdt error\n");
        return -1;
    }
    
    struct descriptor *pdesc = &gdt[index];
    pdesc->base_low = (u32)ptss & 0xffff;
    pdesc->base_mid = ((u32)ptss >> 16) & 0xf;
    pdesc->base_high = (u32)ptss >> 24;

    u32 limit = sizeof(*ptss) - 1;
    pdesc->limit_low = limit & 0xffff;
    pdesc->limit_high_attr2 = (limit >> 16) & 0xf;
    pdesc->attr1 = DA_386TSS | DA_DPL0;

    return index << 3;
}

int uninstall_tss(u16 sel) 
{
    return uninstall_desc(sel);
}

int install_ldt(void *ldt, u16 size) 
{
    int index = alloc();
    if (index > GDT_SIZE - 1)
        return -1;

    struct descriptor *pdesc = &gdt[index];
    pdesc->base_low = (u32)ldt & 0xffff;
    pdesc->base_mid = ((u32)ldt >> 16) & 0xf;
    pdesc->base_high = (u32)ldt >> 24;

    u32 limit = sizeof(struct descriptor) * size - 1;
    pdesc->limit_low = limit & 0xffff;
    pdesc->limit_high_attr2 = (limit >> 16) & 0xf;
    pdesc->attr1 = DA_LDT | DA_DPL0;

    return index << 3;
}

int uninstall_ldt(u16 sel) 
{
    return uninstall_desc(sel);
}
