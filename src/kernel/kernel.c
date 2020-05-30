#include "const.h"
#include "string.h"
#include "type.h"
#include "stdio.h"
#include "unistd.h"
#include "protect.h"
#include "schedule.h"

extern void app1();
extern void app2();

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

void kernel_idle() {
    static int count = 0;
    reset_text_color();
    printf_pos(35, "kernel idle: %d", count++);
}

void init_system() {
    // display banner
    display_banner();

    // create process
    create_proc(app1);
    create_proc(app2);
}