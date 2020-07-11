#include "const.h"
#include "string.h"
#include "type.h"
#include "stdio.h"
#include "unistd.h"
#include "protect.h"
#include "schedule.h"
#include "log.h"
#include "gdt.h"

extern void app1();
extern void app2();
extern void kernel_idle();
extern u32 sys_stacktop;
extern t_proc *current;

TSS tss;
int tss_sel;

static const char *banner = 
  "\n     / /                          //   ) ) //   ) ) \n"
    "    / /         ___      ___     //   / / ((        \n"
    "   / /        //___) ) //___) ) //   / /    \\      \n"
    "  / /        //       //       //   / /       ) )   \n"
    " / /____/ / ((____   ((____   ((___/ / ((___ / /    v0.1\n\n";

static void display_banner() {
    set_text_color(0xa);
    puts(banner);
    reset_text_color();
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

    // init TSS
    tss.ss0 = SELECTOR_KERNEL_DS;
    tss_sel = install_tss(&tss);
    if (tss_sel < 0) {
        log_error("install TSS error: %d\n", tss_sel);
        return -1;
    }
    
    // display banner
    display_banner();

    sleep(1);
    clear_screen();

    current = create_kproc(kernel_idle);
    return 0;
}