#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <config.h>
#include <lib/misc.h>
#include <kernel/log.h>
#include <kernel/kernel.h>
#include <kernel/interrupt.h>
#include <kernel/sched.h>
#include <kernel/memory.h>

#include "page.h"
#include "kmalloc.h"
#include "mm.h"

struct page_entry *alloc_page_table(void)
{
    struct page_entry *dir = kmemalign(PAGE_SIZE, PAGE_SIZE);
    if (dir == NULL) {
        log_error("alloc paging table error\n");
        return NULL;
    }
    memcpy(dir, (void *)virt_addr(CONFIG_KERNEL_PG_ADDR), PAGE_SIZE);
    dir[0].present = 0; // strip 0 ~ 4MB mapping
    return phy_addr(dir);
}

struct page_entry *clone_page_table(const struct page_entry *dir)
{
    uint32_t i;
    struct page_entry *tbl, *cloned;

    cloned = kmemalign(PAGE_SIZE, PAGE_SIZE);
    if (cloned == NULL) {
        return NULL;
    }

    memcpy(cloned, virt_addr(dir), PAGE_SIZE);
    for (i = 0; i < PAGE_ENTRY_COUNT; i++) {
        if (!cloned[i].present || (i<<PGDIR_SHIFT) >= CONFIG_KERNEL_VM_OFFSET) {
            continue;
        }
        if (!cloned[i].present) {
            continue;
        }
        tbl = (void *)(cloned[i].index << PAGE_SHIFT);
        obtain_page((uint32_t)tbl);
        cloned[i].rw = 0;
    }
    return phy_addr(cloned);
}

void free_page_table(struct page_entry *dir)
{
    uint32_t i, j;
    uint32_t page;
    struct page_entry *tbl;

    if (dir == NULL) {
        return;
    }

    for (i = 0; i < PAGE_ENTRY_COUNT; i++) {
        if (!virt_addr(dir)[i].present || (i<<PGDIR_SHIFT) >= CONFIG_KERNEL_VM_OFFSET) {
            continue;
        }
        tbl = (void *)(virt_addr(dir)[i].index << PAGE_SHIFT);
        for (j = 0; j < PAGE_ENTRY_COUNT; j++) {
            if (!virt_addr(tbl)[j].present) {
                continue;
            }
            page = virt_addr(tbl)[j].index << PAGE_SHIFT;
            free_page(page);
            if (page_users(page) == 0) {
                virt_addr(tbl)[j].present = 0;
            }
        }
        free_page((uint32_t)tbl);
        if (page_users((uint32_t)tbl) == 0) {
            virt_addr(dir)[i].present = 0;
        }
    }
    kfree(virt_addr(dir));
}

void mm_setup(void)
{
    log_info("setup memory\n");
    log_info("page size %u bytes\n", PAGE_SIZE);
    init_page();
    init_heap();
}

void mm_report(void)
{
    page_report();
    // heap_report();
}