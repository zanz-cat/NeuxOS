#include "const.h"
#include "kliba.h"
#include "stdio.h"

PUBLIC void init_8259A(){
    puts("Initialize 8259A Interrupt Controller\n");

    out_byte(0x20, 0x11);

    out_byte(0x21, 0x20);
    out_byte(0xa1, 0x28);

    out_byte(0x21, 0x4);
    out_byte(0xa1, 0x2);

    out_byte(0x21, 0x1);
    out_byte(0xa1, 0x1);

    out_byte(0x21, 0xff);
    out_byte(0xa1, 0xff);
}