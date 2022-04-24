#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <drivers/keyboard.h>
#include <drivers/harddisk.h>
#include <arch/x86.h>
#include <fs/ext2.h>
#include <fs/dev.h>
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
    tty_color(tty_get_cur(), TTY_OP_GET, &origin);
    tty_color(tty_get_cur(), TTY_OP_SET, &color);
    printk(banner);
    tty_color(tty_get_cur(), TTY_OP_SET, &origin);
    printk("Welcome to NeuxOS!\n");
}

void kernel_panic(const char *fmt, ...)
{
    uint8_t color, color_fatal;
	va_list args;

    color_fatal = 0x4;
    tty_color(tty_get_cur(), TTY_OP_GET, &color);
    tty_color(tty_get_cur(), TTY_OP_SET, &color_fatal);

    printk("kernel panic! \nError: ");
	va_start(args, fmt);
    vprintk(fmt, args);
	va_end(args);

    tty_color(tty_get_cur(), TTY_OP_SET, &color);
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
    struct file *stdin, *stdout;

    set_log_level(DEBUG);
    mm_setup();
    enable_em();
    irq_setup();
    clock_setup();
    keyboard_setup();
    hd_setup();
    vfs_setup();
    ext2_setup();
    devfs_setup();
    tty_setup();
    sched_setup();

    task = create_ktask("[ttyd]", tty_task);
    if (task == NULL) {
        kernel_panic("create tty task failed\n");
    }
    task_start(task);

    task = create_utask("/bin/init", NULL, NULL, NULL);
    if (task == NULL) {
        kernel_panic("create init task failed\n");
    }
    task_start(task);

    stdin = vfs_open("/dev/ttyS0", R_OK);
    stdout = vfs_open("/dev/ttyS0", W_OK);
    task = create_utask("/bin/nxsh", stdin, stdout, NULL);
    if (task == NULL) {
        kernel_panic("create nxsh task failed\n");
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

static void __attribute__((unused)) open_a_txt()
{
    struct inode *pinode;
    struct file *pfile = vfs_open("/root/a.txt", 0);
    if (pfile == NULL) {
        log_error("open file error: %d\n", errno);
        return;
    }
    pinode = pfile->dent->inode;
    char *buf = kmalloc(pinode->size);
    if (buf == NULL) {
        log_error("malloc failed\n");
        vfs_close(pfile);
        return;
    }
    int ret = vfs_read(pfile, buf, pinode->size);
    printk("a.txt size=%d, content=...%s\n", ret, buf + pinode->size - 16);
    vfs_close(pfile);
}