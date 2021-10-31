#include <string.h>
#include <elf.h>

#include <arch/x86.h>
#include <errcode.h>
#include <fs/ext2.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>

#include "log.h"
#include "descriptor.h"
#include "sched.h"

#include "task.h"

#define INITIAL_EFLAGS (EFLAGS_IF | 0x3000) /* IF | IOPL=3 */

struct _stack_content {
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

static int init_kernel_task(struct task *task, uint32_t text)
{
    int ret;
    void *s0;

    s0 = kmemalign(sizeof(uint32_t), STACK0_SIZE);
    if (s0 == NULL) {
        return -EOOM;
    }
    memset(s0, 0, STACK0_SIZE);

    /* init TSS */
    task->tss.prev = 0;
    task->tss.esp0 = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.ss0 = 0;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = (uint32_t)CONFIG_KERNEL_PG_ADDR;
    task->tss.eip = text;
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.esp = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.ebp = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = SELECTOR_KERNEL_DS;
    task->tss.cs = SELECTOR_KERNEL_CS;
    task->tss.ss = SELECTOR_KERNEL_DS;
    task->tss.ds = SELECTOR_KERNEL_DS;
    task->tss.fs = 0;
    task->tss.gs = 0;
    task->tss.attrs = 0;
    task->tss.io = 0;

    ret = install_tss(&task->tss, DA_DPL0);
    if (ret < 0) {
        kfree(s0);
        return ret;
    }
    task->tss_sel = (uint16_t)ret;
    return 0;
}

static int load_elf(const void *elf, int voffset, uint32_t *entry_point)
{
    int i, count;
    const Elf32_Ehdr *eh;
    Elf32_Phdr *ph;
    Elf32_Shdr *sh;
    Elf32_Addr dest;

    eh = elf;
    if (strncmp((char *)eh->e_ident, ELFMAG, SELFMAG) != 0) {
        return -EINTVAL;
    }
    *entry_point = eh->e_entry;

    count = 0;
    for (i = 0; i < eh->e_phnum; i++) {
        ph = (Elf32_Phdr *)(elf + eh->e_phoff + eh->e_phentsize * i);
        dest = ph->p_vaddr + voffset;
        if (ph->p_type == PT_LOAD && ph->p_filesz > 0 && dest >= 0) {
            memcpy((void *)dest, elf + ph->p_offset, ph->p_filesz);
            count++;
        }
    }
    for (i = 0; i < eh->e_shnum; i++) {
        sh = (Elf32_Shdr *)(elf + eh->e_shoff + eh->e_shentsize * i);
        dest = sh->sh_addr + voffset;
        if (sh->sh_type == SHT_NOBITS && dest >= 0) {
            memset((void *)dest, 0, sh->sh_size);
            count++;
        }
    }
    return count;
}

static void user_task_launcher(void)
{
    uint32_t ebp;
    struct _stack_content *_stack;

    struct ext2_file *f = ext2_open(current->exe);
    if (f == NULL) {
        log_error("open file %s failed\n", current->exe);
        term_task(current);
    }
    char *buf = kmalloc(f->size);
    if (buf == NULL) {
        log_error("malloc failed\n");
        ext2_close(f);
        term_task(current);
    }
    int ret = ext2_read(f, buf, f->size);
    ext2_close(f);
    if (ret < 0) {
        log_error("read error: %d\n", ret);
        kfree(buf);
        term_task(current);
    }

    uint32_t entry_point;
    ret = load_elf(buf, 0, &entry_point);
    kfree(buf);
    if (ret < 0) {
        log_error("load elf error: %d\n", ret);
        term_task(current);
    }

    asm("movl %%ebp, %0":"=r"(ebp)::);
    _stack = (void *)(*(uint32_t *)ebp - sizeof(struct _stack_content));
    _stack->ss = SELECTOR_USER_DS;
    _stack->esp = CONFIG_KERNEL_VM_OFFSET;
    _stack->eflags = INITIAL_EFLAGS;
    _stack->cs = SELECTOR_USER_CS;
    _stack->eip = entry_point;
    asm("leave\n\t"
        "mov %0, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "iret\n\t"::"i"(SELECTOR_USER_DS):);
}

static int init_user_task(struct task *task)
{
    int ret;
    void *s0;

    s0 = kmemalign(sizeof(uint32_t), STACK0_SIZE);
    if (s0 == NULL) {
        return -EOOM;
    }
    memset(s0, 0, STACK0_SIZE);

    /* init TSS */
    task->tss.prev = 0;
    task->tss.esp0 = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.ss0 = SELECTOR_KERNEL_DS;
    task->tss.esp1 = 0;
    task->tss.ss1 = 0;
    task->tss.esp2 = 0;
    task->tss.ss2 = 0;
    task->tss.cr3 = (uint32_t)alloc_user_page();
    task->tss.eip = (uint32_t)user_task_launcher;
    task->tss.eflags = INITIAL_EFLAGS;
    task->tss.eax = 0;
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.ebp = (uint32_t)PTR_ADD(s0, STACK0_SIZE);
    task->tss.esp = task->tss.ebp - sizeof(struct _stack_content);
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = SELECTOR_KERNEL_DS;
    task->tss.cs = SELECTOR_KERNEL_CS;
    task->tss.ss = SELECTOR_KERNEL_DS;
    task->tss.ds = SELECTOR_KERNEL_DS;
    task->tss.fs = 0;
    task->tss.gs = 0;
    task->tss.attrs = 0;
    task->tss.io = 0;

    ret = install_tss(&task->tss, DA_DPL3);
    if (ret < 0) {
        kfree(s0);
        return ret;
    }
    task->tss_sel = (uint16_t)ret;
    return 0;
}

struct task *create_kernel_task(uint32_t text, const char *exe, int tty)
{
    int ret;
    struct task *task;

    task = (struct task *)kmalloc(sizeof(struct task));
    if (task == NULL) {
        return NULL;
    }
    memset(task, 0, sizeof(struct task));
    task->state = TASK_STATE_INIT;
    task->type = TASK_TYPE_KERNEL;
    task->pid = 0;
    task->tty = tty;
    strncpy(task->exe, exe, MAX_PATH_LEN);

    ret = init_kernel_task(task, text);
    if (ret < 0) {
        return NULL;
    }
    task->state = TASK_STATE_RUNNING;
    return task;
}

struct task *create_user_task(const char *exe, int tty)
{
    int ret;
    struct task *task;

    task = (struct task *)kmalloc(sizeof(struct task));
    if (task == NULL) {
        return NULL;
    }
    memset(task, 0, sizeof(struct task));
    task->state = TASK_STATE_INIT;
    task->type = TASK_TYPE_USER;
    task->pid = 0;
    task->tty = tty;
    strncpy(task->exe, exe, MAX_PATH_LEN);

    ret = init_user_task(task);
    if (ret < 0) {
        return NULL;
    }
    task->state = TASK_STATE_RUNNING;
    return task;
}

int destroy_task(struct task *task)
{
    if (task->type == TASK_TYPE_USER) {
        free_user_page((void *)task->tss.cr3);
    }
    uninstall_tss(task->tss_sel);
    kfree(PTR_SUB((void *)task->tss.esp0, STACK0_SIZE));
    kfree(task);
    return 0;
}