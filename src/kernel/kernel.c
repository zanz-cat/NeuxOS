#include "const.h"
#include "string.h"
#include "type.h"
#include "stdio.h"
#include "unistd.h"
#include "protect.h"
#include "schedule.h"

extern void app1();
extern void app2();

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128

u8 gdt_ptr[6];
DESCRIPTOR gdt[GDT_SIZE];

static const char *banner = 
  "\n     / /                          //   ) ) //   ) ) \n"
    "    / /         ___      ___     //   / / ((        \n"
    "   / /        //___) ) //___) ) //   / /    \\      \n"
    "  / /        //       //       //   / /       ) )   \n"
    " / /____/ / ((____   ((____   ((___/ / ((___ / /    v0.1\n\n";

void relocate_gdt() {
    memcpy(&gdt, (void*)(*((u32*)(&gdt_ptr[2]))), *((u16*)(&gdt_ptr[0]))+1);
    u16 *p_gdt_limit = (u16*)(&gdt_ptr[0]);
    u32 *p_gdt_base = (u32*)(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *p_gdt_base = (u32)&gdt;
}

void display_banner() {
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