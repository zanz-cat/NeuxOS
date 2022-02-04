#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <config.h>
#include <misc/misc.h>
#include <misc/misc.h>
#include <kernel/log.h>
#include <kernel/kernel.h>
#include <kernel/printk.h>
#include <kernel/interrupt.h>
#include <kernel/sched.h>
#include <kernel/memory.h>

#include "page.h"
#include "kmalloc.h"
#include "mm.h"

struct page_entry *alloc_user_page(void)
{
    struct page_entry *dir = kmemalign(PAGE_SIZE, PAGE_SIZE);
    if (dir == NULL) {
        log_error("alloc paging table error\n");
        return NULL;
    }
    memcpy(dir, (void *)virt_addr(CONFIG_KERNEL_PG_ADDR), PAGE_SIZE);
    return phy_addr(dir);
}

void free_user_page(struct page_entry *dir)
{
    uint32_t i, j;
    uint32_t page;
    struct page_entry *tbl;

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

#define PAGE_FAULT_NO_PERM 0x1
#define PAGE_FAULT_WRITE 0x2
#define PAGE_FAULT_USER 0x4

void page_fault_handler(uint32_t err_code, uint32_t eip, uint32_t cs, uint32_t eflags)
{
    uint32_t cr2, cr3, page, tpage, i;
    struct page_entry *dir, *tbl;
    const char *reason;
    bool write, user;

    SAVE_STATE();
    asm("movl %%cr2, %0":"=r"(cr2)::);
    write = err_code & PAGE_FAULT_WRITE;
    user = err_code & PAGE_FAULT_USER;
    if (err_code & PAGE_FAULT_NO_PERM) {
        reason = "no permission";
        goto error;
    }
    if (cr2 == 0) {

    }
    if (cr2 >= CONFIG_KERNEL_VM_OFFSET) {
        reason = "never reach";
        goto error;
    }
    page = alloc_page(PGF_NORMAL);
    if (page == -1) {
        reason = "alloc page error";
        goto error;
    }
    asm("movl %%cr3, %0":"=r"(cr3)::);
    dir = (void *)virt_addr(cr3);
    i = cr2 >> PGDIR_SHIFT;
    if (!dir[i].present) {
        tpage = alloc_page(PGF_NORMAL);
        if (tpage == -1) {
            reason = "alloc page error";
            free_page(page);
            goto error;
        }
        memset(virt_addr((void *)tpage), 0, PAGE_SIZE);
        dir[i].rw = 1;
        dir[i].us = current->type == TASK_TYPE_USER;
        dir[i].index = tpage >> PAGE_SHIFT;
        dir[i].present = 1;
    }
    tbl = (void *)virt_addr(dir[i].index << PAGE_SHIFT);
    i = (cr2 >> PAGE_SHIFT) & 0x3ff;
    tbl[i].index = page >> PAGE_SHIFT;
    tbl[i].rw = 1;
    tbl[i].us = current->type == TASK_TYPE_USER;
    tbl[i].present = 1;
    RESTORE_STATE();
    return;
error:
    fprintk(TTY0,"\n[ERROR] page fault, %s\n"
           "> task: %u\n"
           "> exe: %s\n"
           "> eflags: 0x%x\n"
           "> cs: 0x%x\n"
           "> eip: %p\n"
           "> cr2: %p\n"
           "> r/w: %s\n"
           "> user: %d\n\n",
           reason, current->pid, current->exe, 
           eflags, cs, eip, cr2, write ? "w" : "r", user);

    task_term(current);
    task_sched();
    asm("ljmp *(%0)"::"p"(current):);
}

void mm_setup(void)
{
    log_info("setup memory management\n");
    log_info("page size %u bytes\n", PAGE_SIZE);
    init_page();
    init_heap();
}

void mm_report(void)
{
    page_report();
    // heap_report();
}