#include "const.h"
#include "string.h"
#include "type.h"
#include "stdio.h"
#include "unistd.h"
#include "protect.h"
#include "sched.h"
#include "log.h"
#include "gdt.h"

extern void app1();
extern void app2();
extern void kernel_idle();
extern void init_interrupt();
extern t_proc *current;

TSS tss;
int tss_sel;

static const char *banner = 
  "\n     / /                          //   ) ) //   ) ) \n"
    "    / /         ___      ___     //   / / ((        \n"
    "   / /        //___) ) //___) ) //   / /    \\      \n"
    "  / /        //       //       //   / /       ) )   \n"
    " / /____/ / ((____   ((____   ((___/ / ((___ / /    v0.01\n\n";

static void display_banner() {
    set_text_color(0xa);
    puts(banner);
    reset_text_color();
}

static void init_timer() {
    log_info("init system timer\n");
    out_byte(TIMER_MODE, RATE_GENERATOR);
    out_byte(TIMER0, (u8)(TIMER_FREQ/HZ));
    out_byte(TIMER0, (u8)((TIMER_FREQ/HZ) >> 8));
}

void _idle() {
    static int count = 0;
    if (count++ % 100) {
        return;
    }
    set_text_color(0x2);
    printf_pos(60, "kernel idle: %d", count / 100);
    reset_text_color();
}

int init_system() {
    set_log_level(DEBUG);

    init_interrupt();

    init_timer();

    // init TSS
    tss.ss0 = SELECTOR_KERNEL_DS;
    tss_sel = install_tss(&tss);
    if (tss_sel < 0) {
        log_error("install TSS error: %d\n", tss_sel);
        return -1;
    }
    
    // display banner
    display_banner();

    // clear_screen();

    current = create_kproc(kernel_idle);
    return 0;
}