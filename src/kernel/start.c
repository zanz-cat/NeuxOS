#include "const.h"
#include "string.h"
#include "type.h"
#include "stdio.h"
#include "unistd.h"

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128
#define IDT_SIZE    256

u8 gdt_ptr[6];
DESCRIPTOR gdt[GDT_SIZE];

u8 idt_ptr[6];
GATE idt[IDT_SIZE];

const char *banner[] = {
    "\n     / /                          //   ) ) //   ) ) \n",
    "    / /         ___      ___     //   / / ((        \n",
    "   / /        //___) ) //___) ) //   / /    \\      \n",
    "  / /        //       //       //   / /       ) )   \n",
    " / /____/ / ((____   ((____   ((___/ / ((___ / /    v0.1\n\n",
    NULL
};

void cstart() {
    memcpy(&gdt, (void*)(*((u32*)(&gdt_ptr[2]))), *((u16*)(&gdt_ptr[0]))+1);
    u16 *p_gdt_limit = (u16*)(&gdt_ptr[0]);
    u32 *p_gdt_base = (u32*)(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *p_gdt_base = (u32)&gdt;

    u16 *p_idt_limit = (u16*)idt_ptr;
    u32 *p_idt_base = (u32*)(idt_ptr+2);
    *p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
    *p_idt_base = (u32)&idt;

    for (int i = 1, len = 0; i <= 100; i++) {
        for (int j = 0; j < len; j++) {
            backspace();
        }
        len = printf("Starting...%d%", i);
        usleep(2);
    }
    putchar('\n');

    set_text_color(0xa);
    for (const char** s = banner; *s != NULL; s++) {
        puts(*s);
    }

    reset_text_color();
    puts("liwei-PC login: ");
}
