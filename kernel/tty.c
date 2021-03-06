#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <drivers/io.h>
#include <drivers/monitor.h>
#include <drivers/keyboard.h>
#include <dev/dev.h>
#include <mm/kmalloc.h>

#include "kernel.h"
#include "log.h"
#include "printk.h"
#include "sched.h"

#include "tty.h"

extern void mm_report(void);
extern void sched_report(void);

#define TTYS_COUNT (TTY_MAX+1)
#define SCROLL_ROWS 15
#define TTY_BUF_SIZE 1024

struct iobuf {
    char data[TTY_BUF_SIZE];
    size_t count;
    char *head;
    char *tail;
};

static inline void iobuf_init(struct iobuf *buf)
{
    buf->count = 0;
    buf->head = buf->data;
    buf->tail = buf->data;
}

static inline void iobuf_put(struct iobuf *buf, char ch)
{
    *(buf->head++) = ch;
    if (buf->head == buf->data + TTY_BUF_SIZE) {
        buf->head = buf->data;
    }
    buf->count++;
}

static inline int iobuf_get(struct iobuf *buf, char *ch)
{
    if (buf->count == 0) {
        return -ENOENT;
    }
    *ch = *(buf->tail++);
    if (buf->tail == buf->data + TTY_BUF_SIZE) {
        buf->tail = buf->data;
    }
    buf->count--;
    return 0;
}

struct tty {
    struct iobuf in;
    struct monitor mon;
};

static struct tty ttys[TTYS_COUNT];
static int _tty_current;

static void tty_switch(int fd)
{
    monitor_switch(&ttys[_tty_current].mon, &ttys[fd].mon);
    _tty_current = fd;
}

static void putctrl(int fd)
{
    iobuf_put(&ttys[fd].in, '\033');
    iobuf_put(&ttys[fd].in, '[');
}

static void tty_in_proc(int fd, uint16_t key)
{
    if (fd < TTY0 || fd > TTY_MAX) {
        return;
    }

    if (key & FLAG_EXT) {
        switch (key) {
            case ENTER:
                key = '\n';
                goto put_in_buf;
            case BACKSPACE:
                key = '\b';
                goto put_in_buf;
            case FLAG_SHIFT_L | PAGEUP:
                for (int i = 0; i < SCROLL_ROWS; i++) {
                    monitor_scroll_up(&ttys[fd].mon);
                }
                break;
            case FLAG_SHIFT_L | PAGEDOWN:
                for (int i = 0; i < SCROLL_ROWS; i++) {
                    monitor_scroll_down(&ttys[fd].mon);
                }
                break;
            case FLAG_SHIFT_L | F1:
                tty_switch(TTY0);
                break;
            case FLAG_SHIFT_L | F2:
                tty_switch(TTY1);
                break;
            case FLAG_SHIFT_L | F3:
                tty_switch(TTY2);
                break;
            case FLAG_SHIFT_L | F4:
                break;
            case FLAG_SHIFT_L | F5:
                mm_report();
                sched_report();
                break;
            case UP:
            case DOWN:
            case LEFT:
            case RIGHT:
                putctrl(fd);
                iobuf_put(&ttys[fd].in, 'A' + (key - UP));
                break;
            default:
                break;
        }
        return;
    }

put_in_buf:
    iobuf_put(&ttys[fd].in, (char)key);
}

static void tty_do_read(int fd)
{
    if (fd != _tty_current) {
        return;
    }
    keyboard_read(tty_in_proc, fd);
}

static ssize_t tty_write(int fd, const char *buf, size_t n)
{
    int i;

    for (i = 0; i < n; i++) {
        monitor_putchar(&ttys[fd].mon, buf[i]);
    }
    return i;
}

int tty_color(int fd, enum tty_op op, uint8_t *color)
{
    if (fd == TTY_NULL) {
        return 0;
    }
    if (fd < TTY_NULL || fd > TTY_MAX) {
        log_error("invalid tty %d\n", fd);
        return -1;
    }

    struct monitor *mon = &ttys[fd].mon;

    if (op == TTY_OP_GET) {
        *color = mon->color;
    } else {
        mon->color = *color;
    }
    return 0;
}

void tty_task()
{
    int i;

    while (1) {
        for (i = 0; i < TTYS_COUNT; i++) {
            tty_do_read(i);
        }
    }
}

int tty_get_cur()
{
    return _tty_current;
}

static ssize_t tty_dev_write(struct dev *devp, const char *buf, size_t count)
{
    return tty_write((int)devp->priv, buf, count);
}

static ssize_t tty_dev_read(struct dev *devp, char *buf, size_t count)
{
    int i, fd, ret;

    i = 0;
    fd = (int)devp->priv;
    while (i < count) {
        ret = iobuf_get(&ttys[fd].in, buf + i);
        if (ret == 0) {
            i++;
            continue;
        }
        if (i > 0) {
            break;
        }
        yield();
    }
    return i;
}

static struct dev *create_tty_dev(int fd)
{
    static struct cdev_ops tty_ops = {
        .open = NULL,
        .close = NULL,
        .write = tty_dev_write,
        .read = tty_dev_read,
    };

    struct dev *devp;
    devp = kmalloc(sizeof(struct dev));
    if (devp == NULL) {
        errno = -ENOMEM;
        return NULL;
    }
    dev_init(devp);
    devp->dno = fd;
    devp->type = DEV_T_CDEV;
    devp->ops.cdev = &tty_ops;
    devp->priv = (void *)fd;
    return devp;
}

static void ttydev_setup(void)
{
    int i, ret;
    struct dev *devp;
    char *path = "/dev/ttyS0";

    for (i = TTY0; i < TTYS_COUNT; i++) {
        devp = create_tty_dev(i);
        if (devp == NULL) {
            ret = errno;
            goto error;
        }
        ret = dev_add(devp);
        if (ret != 0) {
            goto error;
        }
        ret = devfs_mknod(path, devp->dno);
        if (ret != 0) {
            goto error;
        }
        path[strlen(path) - 1]++;
    }
    return;

error:
    kernel_panic("mknod %s error=%d\n", path, ret);
}

static void tty_monitor_init(void)
{
    int i;
    for (i = TTY0; i < TTYS_COUNT; i++) {
        iobuf_init(&ttys[i].in);
        monitor_init(&ttys[i].mon, i);
    }
    monitor_switch(NULL, &ttys[TTY0].mon);
}

void tty_setup()
{
    tty_monitor_init();
    ttydev_setup();
    printk_set_write(tty_write);
}

