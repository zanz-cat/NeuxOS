#include "keyboard.h"
#include "stdio.h"
#include "type.h"

void task_tty() {
    while (1) {
        keyboard_read();
    }
}

void in_process(u32 key) {
    if (key & FLAG_EXT) {
        switch (key) {
        case ENTER:
            printf("\n");
            break;
        case ESC:
            clear_screen();
            break;
        default:
            break;
        }

        return;
    }
    printf("%c", (u8)key);
}