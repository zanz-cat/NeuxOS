#include "stdio.h"
#include "unistd.h"

void app1() {
    static int count = 0;
    while (1) {
        set_text_color(0x4);
        printf_pos(935, "app1: %d", get_ticks());
        reset_text_color();
    }
}

void app2() {
    static int count = 0;
    while (1) {
        set_text_color(0x2);
        printf_pos(1015, "app2: %d", count++);
        reset_text_color();
    }
}

void kapp1() {
    static int count = 0;
    while (1) {
        set_text_color(0x1);
        printf_pos(1095, "kapp1: %d", count++);
        reset_text_color();
    }
}

void kapp2() {
    static int count = 0;
    while (1) {
        set_text_color(0x3);
        printf_pos(1175, "kapp2: %d", count++);
        reset_text_color();
    }
}