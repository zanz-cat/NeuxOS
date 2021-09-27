#include <stdint.h>
#include <string.h>

#include <drivers/keyboard.h>
#include <drivers/harddisk.h>
#include <arch/x86.h>
#include <fs/ext2.h>

#include "log.h"
#include "mm.h"
#include "sched.h"
#include "gdt.h"
#include "clock.h"
#include "kmalloc.h"
#include "printk.h"
#include "tty.h"
#include "interrupt.h"

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
    set_log_level(DEBUG);
    tty_setup();
    mm_setup();
    sched_setup();
    enable_em();
    irq_setup();
    clock_setup();
    keyboard_setup();
    hd_setup();
    ext2_setup();

    create_kernel_task(tty_task, "[ttyd]", TTY0);
    welcome();
}

void kernel_loop(void)
{
    uint32_t count = 0;
    struct paging_entry *pg_dir;

    // some cleaning jobs
    pg_dir = (void *)(CONFIG_KERNEL_PG_ADDR + CONFIG_KERNEL_VMA);
    pg_dir[0].present = 0;

    enable_irq();
    while (1) {
        if (count++ % 100000 == 0) {
            sched_report();
        }
        asm("hlt");
    }
}

void F1_handler(void)
{
    // int sect_cnt = 10;
    // char buf2[CONFIG_HD_SECT_SZ * sect_cnt];
    // hd_read(2048, sect_cnt, buf2, sizeof(buf2));
    // printk("buf2: %s\n", buf2);
    // return;

    struct ext2_file *f = ext2_open("/bin/app");
    if (f == NULL) {
        printk("open file failed\n");
        return;
    }
    char *buf = kmalloc(f->size);
    if (buf == NULL) {
        printk("malloc failed\n");
        return;
    }
    int ret = ext2_read(f, buf, f->size);
    if (ret < 0) {
        printk("read error: %d\n", ret);
    }
    // create_user_task();
    printk(buf);
    ext2_close(f);
    kfree(buf);
}