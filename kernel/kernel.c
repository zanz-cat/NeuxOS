#include <stdint.h>
#include <string.h>

#include <drivers/keyboard.h>
#include <drivers/harddisk.h>
#include <arch/x86.h>
#include <fs/ext2.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <fs/fs.h>

#include "log.h"
#include "sched.h"
#include "descriptor.h"
#include "clock.h"
#include "printk.h"
#include "tty.h"
#include "interrupt.h"
#include "task.h"
#include "memory.h"

/**
 * https://www.bootschool.net/ascii
 * font: 5lineoblique
 **/
static const char *banner = "\0"
  "\n     /|    / /                           //   ) ) //   ) ) \n"
    "    //|   / /  ___                      //   / / ((        \n"
    "   // |  / / //___) ) //   / / \\\\  //  //   / /    \\\\      \n"
    "  //  | / / //       //   / /   \\\\//  //   / /       ) )   \n"
    " //   |/ / ((____   ((___( (    //\\\\ ((___/ / ((___ / /    v0.03\n\n";

static void welcome()
{
    uint8_t color = 0xa;
    uint8_t origin;
    tty_color(tty_current, TTY_OP_GET, &origin);
    tty_color(tty_current, TTY_OP_SET, &color);
    printk(banner);
    tty_color(tty_current, TTY_OP_SET, &origin);
    printk("Welcome to NeuxOS!\n");
}

void kernel_panic(const char *fmt, ...)
{
    uint8_t color, color_fatal;
	va_list args;

    color_fatal = 0x4;
    tty_color(tty_current, TTY_OP_GET, &color);
    tty_color(tty_current, TTY_OP_SET, &color_fatal);

    printk("kernel panic! \nError: ");
	va_start(args, fmt);
    vprintk(fmt, args);
	va_end(args);

    tty_color(tty_current, TTY_OP_SET, &color);
    asm("hlt");
}

void enable_em(void)
{
    return;
    asm("movl %%cr0, %%eax\n\t"
        "or %0, %%eax\n\t"
        "movl %%eax, %%cr0"
        :
        :"i"(0x4)
        :"%eax");
}

void kernel_setup()
{
    struct task *task;

    set_log_level(DEBUG);
    tty_setup();
    mm_setup();
    enable_em();
    irq_setup();
    clock_setup();
    keyboard_setup();
    hd_setup();
    vfs_setup();
    ext2_setup();
    sched_setup();

    task = create_user_task("/bin/init", TTY0);
    if (task == NULL) {
        kernel_panic("create init task failed\n");
    }
    task_start(task);

    task = create_kernel_task(tty_task, "[ttyd]", TTY0);
    if (task == NULL) {
        kernel_panic("create tty task failed\n");
    }
    task_start(task);

    welcome();
}

void kernel_loop(void)
{
    struct page_entry *pgdir;

    // some cleaning jobs
    pgdir = (void *)virt_addr(CONFIG_KERNEL_PG_ADDR);
    pgdir[0].present = 0;

    enable_irq();
    while (1) {
        asm("hlt");
    }
}

void F1_handler(void)
{
    struct task *task = create_user_task("/bin/app", TTY1);
    if (task == NULL) {
        printk("create user task failed\n");
        return;
    }
    task_start(task);
    return;

    #define TASK_NUM 10
    static struct task *task_list[TASK_NUM];
    static int i;

    int k = i++ % TASK_NUM;

    if (task_list[k] != NULL) {
        task_term(task_list[k]);
        task_list[k] = NULL;
        return;
    }

    task_list[k] = create_user_task("/bin/app", TTY1);
    if (task_list[k] == NULL) {
        printk("create user task failed\n");
        return;
    }
    task_start(task_list[k]);
}