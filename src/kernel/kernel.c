#include <type.h>
#include <lib/log.h>
#include <lib/string.h>
#include <lib/print.h>
#include <kernel/const.h>
#include <kernel/protect.h>
#include <kernel/sched.h>
#include <kernel/gdt.h>
#include <kernel/clock.h>
#include <kernel/keyboard.h>
#include <kernel/tty.h>

extern void app1();
extern void app2();
extern void kapp1();
extern void kernel_idle();
extern void init_interrupt();

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

char test[10240] = "test";

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
        fprintk(current_console, "idle: %d\n", kget_ticks());
        current_console->color = DEFAULT_TEXT_COLOR;
        asm("hlt");
    }
}

void init_system() 
{
    set_log_level(DEBUG);

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