#include <stdint.h>
#include <string.h>

#include <drivers/monitor.h>
#include <arch/x86.h>
#include <config.h>

#include "log.h"
#include "gdt.h"

#define	GDT_SIZE 8192

struct gdtr {
    uint16_t limit;
    void *base;
} __attribute__((packed));

static struct descriptor gdt[GDT_SIZE];
static uint8_t bitmap[GDT_SIZE/8];

#define BITMAP_SET(index) bitmap[(index)/8] |= (1 << ((index)%8))
#define BITMAP_CLR(index) bitmap[(index)/8] &= (~(1 << ((index)%8)))
#define BITMAP_GET(index) (bitmap[(index)/8] & (1 << ((index)%8)))

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

static void kfree(int index)
{
    if (index > GDT_SIZE - 1)
        return;

    BITMAP_CLR(index);
}

static void install_desc(uint16_t index, uint32_t addr, 
            uint32_t limit, uint16_t attr_type)
{
    struct descriptor *pdesc = &gdt[index];
    pdesc->base_low = (uint32_t)addr & 0xffff;
    pdesc->base_mid = ((uint32_t)addr >> 16) & 0xff;
    pdesc->base_high = (uint32_t)addr >> 24;

    pdesc->limit_low = limit & 0xffff;
    pdesc->limit_high_attr2 = ((attr_type >> 8) & 0xf0) | ((limit >> 16) & 0xf);
    pdesc->attr1 = attr_type & 0xff;
}

static void uninstall_desc(uint16_t index)
{
    memset(&gdt[index], 0, sizeof(struct descriptor));
    kfree(index);
}

void gdt_setup()
{
    struct gdtr gdtr;

    install_desc(SELECTOR_DUMMY >> 3, 0, 0, 0);
    BITMAP_SET(SELECTOR_DUMMY >> 3);

    install_desc(SELECTOR_FLAT_C >> 3, 0, 0xfffff, DA_CR|DA_32|DA_LIMIT_4K);
    BITMAP_SET(SELECTOR_FLAT_C >> 3);

    install_desc(SELECTOR_FLAT_RW >> 3, 0, 0xfffff, DA_DRW|DA_32|DA_LIMIT_4K);
    BITMAP_SET(SELECTOR_FLAT_RW >> 3);

    gdtr.base = &gdt;
    gdtr.limit = GDT_SIZE * sizeof(struct descriptor) - 1;
    asm("lgdt %0"::"m"(gdtr):);
}

// can i install TSS into LDT? No! P289
int install_tss(struct tss *ptss)
{
    int index = alloc();
    if (index < 0) {
        log_error("alloc gdt error\n");
        return -1;
    }
    install_desc(index, (uint32_t)ptss, sizeof(struct tss) - 1, DA_386TSS | DA_DPL0);
    return index << 3;
}

int uninstall_tss(uint16_t sel)
{
    int index = sel >> 3;
    if (index > GDT_SIZE - 1) {
        return -1;
    }
    uninstall_desc(index);
    return 0;
}

int install_ldt(void *ldt, uint16_t size)
{
    int index = alloc();
    if (index > GDT_SIZE - 1) {
        return -1;
    }
    install_desc(index, (uint32_t)ldt, sizeof(struct descriptor) * size - 1, DA_LDT | DA_DPL0);
    return index << 3;
}

int uninstall_ldt(uint16_t sel)
{
    int index = sel >> 3;
    if (index > GDT_SIZE - 1) {
        return -1;
    }
    uninstall_desc(index);
    return 0;
}