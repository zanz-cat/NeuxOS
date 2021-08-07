#ifndef __TTY_H__
#define __TTY_H__
#include <stdint.h>

#define TTY0      0
#define TTY1      1
#define TTY2      2

enum tty_op {
    TTY_OP_SET,
    TTY_OP_GET,
};

void tty_init();
void tty_task();
void tty_in_process(int fd, uint32_t key);
int tty_putchar(int fd, char c);
int tty_color(int fd, enum tty_op op, uint8_t *color);

extern int tty_current;

#endif
