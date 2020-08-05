#include "stdio.h"
#include "unistd.h"

void app1() 
{
    static int count = 0;
    struct console *console = get_console(TTY2_INDEX);
    console->color = 0x4;
    while (1) {
        sleep(2);
        fprintf(console, "app1: %d\n", count++);
    }
}

void app2() 
{
    static int count = 0;
    struct console *console = get_console(TTY3_INDEX);
    console->color = 0x2;
    while (1) {
        sleep(4);
        fprintf(console, "app2: %d\n", count++);
    }
}

void kapp1() 
{
    static int count = 0;
    struct console *console = get_console(TTY3_INDEX);
    console->color = 0x1;    
    while (1)
        fprintf(console, "kapp1: %d\n", count++);
}

void kapp2() 
{
    static int count = 0;
    struct console *console = get_console(TTY3_INDEX);
    console->color = 0x3;
    while (1)
        fprintf(console, "kapp2: %d\n", count++);
}