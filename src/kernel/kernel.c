#include "const.h"
#include "string.h"
#include "type.h"
#include "stdio.h"
#include "unistd.h"
#include "protect.h"

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128

u8 gdt_ptr[6];
DESCRIPTOR gdt[GDT_SIZE];

static const char *banner[] = {
  "\n     / /                          //   ) ) //   ) ) \n",
    "    / /         ___      ___     //   / / ((        \n",
    "   / /        //___) ) //___) ) //   / /    \\      \n",
    "  / /        //       //       //   / /       ) )   \n",
    " / /____/ / ((____   ((____   ((___/ / ((___ / /    v0.1\n\n",
    NULL
};

void relocate_gdt() {
    memcpy(&gdt, (void*)(*((u32*)(&gdt_ptr[2]))), *((u16*)(&gdt_ptr[0]))+1);
    u16 *p_gdt_limit = (u16*)(&gdt_ptr[0]);
    u32 *p_gdt_base = (u32*)(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *p_gdt_base = (u32)&gdt;
}

void display_banner() {
    set_text_color(0xa);
    for (const char** s = banner; *s != NULL; s++) {
        puts(*s);
    }
    reset_text_color();
    
    puts("liwei-pc login: ");
}