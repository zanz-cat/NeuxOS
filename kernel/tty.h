#ifndef __KERNEL_TTY_H__
#define __KERNEL_TTY_H__
#include <stdint.h>

#define TTY_NULL (-1)
#define TTY0 0
#define TTY1 1
#define TTY2 2
#define TTY_MAX TTY2

enum tty_op {
    TTY_OP_SET,
    TTY_OP_GET,
};

void tty_setup();
void tty_task();
ssize_t tty_write(int fd, char *buf, size_t n);
ssize_t tty_read(int fd, char *buf, size_t n);
int tty_color(int fd, enum tty_op op, uint8_t *color);
int tty_get_cur();

#endif
