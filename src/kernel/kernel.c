#include "const.h"
#include "string.h"
#include "type.h"
#include "print.h"
#include "protect.h"
#include "sched.h"
#include "log.h"
#include "gdt.h"
#include "clock.h"
#include "keyboard.h"
#include "tty.h"

extern void app1();
extern void app2();
extern void kapp1();
extern void kernel_idle();
extern void init_interrupt();

struct tss tss;

static const char *banner = 
  "\n    //   ) )                    //   ) ) //   ) ) \n"
    "   ((         ___      ___     //   / / ((        \n"
    "     \\     //___) ) //___) ) //   / /    \\      \n"
    "       ) ) //       //       //   / /       ) )   \n"
    "((___ / / ((____   ((____   ((___/ / ((___ / /    v0.02\n\n";

static void display_banner() 
{
    current_console->color = 0xa;
    putsk(banner);
    current_console->color = DEFAULT_TEXT_COLOR;
}

static void idle() 
{
    while (1) {
        current_console->color = 0x2;
        printk("idle: %d\n", kget_ticks());
        current_console->color = DEFAULT_TEXT_COLOR;
        asm("hlt");
    }
}

void init_system() 
{
    // set_log_level(DEBUG);

    init_console();
    init_tty();
    display_banner();
    init_interrupt();
    init_clock();
    init_keyboard();

    // init struct tss
    tss.ss0 = SELECTOR_KERNEL_DS;
    int tss_sel = install_tss(&tss);
    if (tss_sel < 0) {
        log_fatal("install struct tss error: %d\n", tss_sel);
        asm("hlt");
    }
    asm("ltr %0"::"m"(tss_sel):);

    current = create_kproc(idle, NULL);
    create_kproc(task_tty, NULL);
    create_proc(app1, get_tty(TTY2_INDEX));
    create_proc(app2, get_tty(TTY3_INDEX));

    log_info("system kernel started, launching procs\n");
}