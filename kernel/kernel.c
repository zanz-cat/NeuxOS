#include <stdint.h>
#include <string.h>

#include <lib/log.h>
#include <drivers/keyboard.h>

#include "x86.h"
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
static const char *banner =
  "\n     /|    / /                          //   ) ) //   ) ) \n"
    "    //|   / /  ___                     //   / / ((        \n"
    "   // |  / / //___) ) //   / / \\\\ //  //   / /    \\\\      \n"
    "  //  | / / //       //   / /   \\//  //   / /       ) )   \n"
    " //   |/ / ((____   ((___( (    //\\ ((___/ / ((___ / /    v0.03\n\n";

static void display_banner()
{
    uint8_t color = 0xa;
    uint8_t origin;
    tty_color(tty_current, TTY_OP_GET, &origin);
    tty_color(tty_current, TTY_OP_SET, &color);
    printk(banner);
    tty_color(tty_current, TTY_OP_SET, &origin);
}

static void kernel_idle()
{
    while (1) {
        uint8_t color = 0x2;
        uint8_t origin;
        tty_color(tty_current, TTY_OP_GET, &origin);
        tty_color(tty_current, TTY_OP_SET, &color);
        printk("kernel idle, jeffies: %u\n", kget_jeffies());
        tty_color(tty_current, TTY_OP_SET, &origin);
        asm("hlt");
    }
}

void kernel_init()
{
    set_log_level(DEBUG);

    tty_init();
    display_banner();
    interrupt_init();
    clock_init();
    keyboard_init();

    current = create_kernel_task(kernel_idle, TTY0);
    create_kernel_task(tty_task, TTY0);
    create_user_task(app1, TTY1);
    create_user_task(app2, TTY2);

    log_info("kernel started\n");
}