#include <type.h>

#include <lib/log.h>
#include <lib/string.h>

#include <kernel/const.h>
#include <kernel/protect.h>
#include <kernel/sched.h>
#include <kernel/gdt.h>
#include <kernel/clock.h>
#include <kernel/keyboard.h>
#include <kernel/printk.h>
#include <kernel/tty.h>
#include <kernel/interrupt.h>

extern void app1();
extern void app2();
extern void kapp1();

struct tss tss;

/**
 * https://www.bootschool.net/ascii
 * font: 5lineoblique
 **/
static const char *banner = 
  "\n     /|    / /                          //   ) ) //   ) ) \n"
    "    //|   / /  ___                     //   / / ((        \n"
    "   // |  / / //___) ) //   / / \\\\ //  //   / /    \\\\      \n"
    "  //  | / / //       //   / /   \\//  //   / /       ) )   \n"
    " //   |/ / ((____   ((___( (    //\\ ((___/ / ((___ / /    v0.02\n\n";

static void display_banner() 
{
    u8 color = 0xa;
    tty_color(tty_current, TTY_OP_SET, &color);
    printk(banner);
    color = DEFAULT_TEXT_COLOR;
    tty_color(tty_current, TTY_OP_SET, &color);
}

static void kernel_idle() 
{
    while (1) {
        u8 color = 0x2;
        tty_color(tty_current, TTY_OP_SET, &color);
        printk("kernel_idle: %d\n", kget_ticks());
        color = DEFAULT_TEXT_COLOR;
        tty_color(tty_current, TTY_OP_SET, &color);
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

    // init struct tss
    tss.ss0 = SELECTOR_KERNEL_DS;
    int tss_sel = install_tss(&tss);
    if (tss_sel < 0) {
        log_fatal("install struct tss error: %d\n", tss_sel);
        asm("hlt");
    }
    asm("ltr %0"::"m"(tss_sel):);

    current = create_kproc(kernel_idle, TTY0);
    create_kproc(tty_task, TTY0);
    create_proc(app1, TTY1);
    create_proc(app2, TTY2);

    log_info("system kernel started, launching procs\n");
}