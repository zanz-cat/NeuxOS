#include "stdio.h"
#include "unistd.h"

void app1() {
    static int count = 0;
    while (1) {
        set_text_color(0x4);
        printf_pos(915, "app1: %d", count++);
    }
}

void app2() {
    static int count = 0;
    while (1) {
        set_text_color(0x2);
        printf_pos(1075, "app2: %d", count++);
    }
}