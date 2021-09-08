#include <stdint.h>
#include <string.h>

#include <lib/log.h>
#include <drivers/keyboard.h>

#include <arch/x86.h>
#include "mm.h"
#include "sched.h"
#include "gdt.h"
#include "clock.h"
#include "printk.h"
#include "tty.h"
#include "interrupt.h"

extern void app1();
extern void app2();
extern void kapp1();

/**
 * https://www.bootschool.net/ascii
 * font: 5lineoblique
 **/
static const char *banner = "\0"
  "\n     /|    / /                          //   ) ) //   ) ) \n"
    "    //|   / /  ___                     //   / / ((        \n"
    "   // |  / / //___) ) //   / / \\\\ //  //   / /    \\\\      \n"
    "  //  | / / //       //   / /   \\//  //   / /       ) )   \n"
    " //   |/ / ((____   ((___( (    //\\ ((___/ / ((___ / /    v0.03\n\n";

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

void kernel_basic_setup()
{
    set_log_level(DEBUG);
    tty_setup();
    mm_setup();
    sched_setup();
}

void kernel_main(void)
{
    enable_em();
    irq_setup();
    clock_setup();
    keyboard_setup();

    create_kernel_task(tty_task, TTY0);
    // create_user_task(app1, TTY1);
    // create_user_task(app2, TTY2);
    welcome();
    // mm_report();
    irq_enable();
    while (1) {
        asm("hlt");
    }
}