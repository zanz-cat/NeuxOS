#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/printk.h>

#include "log.h"

static enum log_level sys_level = INFO;
static const char* log_level_name[] = {
    [DEBUG] = "DEBUG",
    [INFO] = "INFO",
    [WARN] = "WARN",
    [ERROR] = "ERROR",
    [FATAL] = "FATAL",
};

static int _log(enum log_level level, const char *fmt, va_list args)
{
    int ret;

    if (level < sys_level) {
        return 0;
    }

    if (level > FATAL) {
        return -1;
    }
    ret = fprintk(TTY0, "[%s] ", log_level_name[level]);
    if (ret < 0) {
        return ret;
    }
    return vfprintk(TTY0, fmt, args);
}

int set_log_level(enum log_level level)
{
    sys_level = level;
    return 0;
}

int log_debug(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = _log(DEBUG, fmt, args);
	va_end(args);
    return ret;
}

int log_info(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = _log(INFO, fmt, args);
	va_end(args);
    return ret;
}

int log_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = _log(ERROR, fmt, args);
	va_end(args);
    return ret;
}

int log_fatal(const char *fmt, ...)
{
    int ret;
    uint8_t color, color_fatal;
	va_list args;

    color_fatal = 0x4;
    tty_color(tty_get_cur(), TTY_OP_GET, &color);
    tty_color(tty_get_cur(), TTY_OP_SET, &color_fatal);

	va_start(args, fmt);
	ret = _log(FATAL, fmt, args);
	va_end(args);

    tty_color(tty_get_cur(), TTY_OP_SET, &color);
    return ret;
}