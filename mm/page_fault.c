#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <config.h>
#include <kernel/printk.h>
#include <kernel/tty.h>
#include <kernel/sched.h>
#include <arch/backtrace.h>
#include <arch/x86.h>

#include "mm.h"
#include "page.h"

#define MAX_BACKTRACE_SIZE 10
#define PAGE_FAULT_NO_PERM 0x1
#define PAGE_FAULT_WRITE 0x2
#define PAGE_FAULT_USER 0x4

static void print_backtrace(void **buf, size_t count)
{
    int i;

    fprintk(TTY0, "\n");
    for (i = 0; i < count; i++) {
        fprintk(TTY0, "#%d %p\n", i, buf[i]);
    }
}

static const char *map_page(uint32_t cr2)
{
    uint32_t page, tpage, cr3, i;
    struct page_entry *dir, *tbl;

    page = alloc_page(PGF_NORMAL);
    if (page == -1) {
        return "alloc page error";
    }
    asm("movl %%cr3, %0":"=r"(cr3)::);
    dir = (void *)virt_addr(cr3);
    i = cr2 >> PGDIR_SHIFT;
    if (!dir[i].present) {
        tpage = alloc_page(PGF_NORMAL);
        if (tpage == -1) {
            free_page(page);
            return "alloc page error";
        }
        memset(virt_addr((void *)tpage), 0, PAGE_SIZE);
        dir[i].rw = 1;
        dir[i].us = current->type == TASK_T_USER;
        dir[i].index = tpage >> PAGE_SHIFT;
        dir[i].present = 1;
    }
    tbl = (void *)virt_addr(dir[i].index << PAGE_SHIFT);
    i = (cr2 >> PAGE_SHIFT) & 0x3ff;
    tbl[i].index = page >> PAGE_SHIFT;
    tbl[i].rw = 1;
    tbl[i].us = current->type == TASK_T_USER;
    tbl[i].present = 1;
    return NULL;
}

void page_fault_handler(uint32_t err_code, uint32_t eip, uint32_t cs, uint32_t eflags)
{
    int ret;
    uint32_t cr2;
    const char *reason;
    bool write, user;
    void *buf[MAX_BACKTRACE_SIZE];

    SAVE_STATE();
    asm("movl %%cr2, %0":"=r"(cr2)::);
    if (cr2 < PAGE_SIZE) {
        reason = "~4k memory access denied";
        goto error;
    }

    write = err_code & PAGE_FAULT_WRITE;
    user = err_code & PAGE_FAULT_USER;
    if (err_code & PAGE_FAULT_NO_PERM) {
        reason = "no permission";
        goto error;
    }
    if (cr2 >= CONFIG_KERNEL_VM_OFFSET) {
        reason = "never reach";
        goto error;
    }
    reason = map_page(cr2);
    if (reason != NULL) {
        goto error;
    }
    RESTORE_STATE();
    return;

error:
    fprintk(TTY0, "\n"
           "[ERROR] page fault, %s\n"
           "> task: %u\n"
           "> exe: %s\n"
           "> eflags: 0x%x\n"
           "> cs: 0x%x\n"
           "> eip: %p\n"
           "> cr2: %p\n"
           "> r/w: %s\n"
           "> user: %s\n"
           "> backtrace: ",
           reason, current->pid, task_name(current), 
           eflags, cs, eip, cr2, write ? "w" : "r", user ? "1" : "0");

    ret = backtrace(buf, MAX_BACKTRACE_SIZE);
    if (ret > 0) {
        print_backtrace(buf, ret);
    } else {
        fprintk(TTY0, "n/a\n");
    }
    task_term(current);
    asm("ljmp *(%0)"::"p"(current):);
}