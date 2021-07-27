#include <lib/stdio.h>
#include <kernel/tty.h>
#include <kernel/printk.h>

#include <lib/log.h>

static enum log_level sys_level = INFO;
static const char* log_level_name[] = {
    [DEBUG] = "DEBUG",
    [INFO] = "INFO",
    [WARN] = "WARN",
    [ERROR] = "ERROR",
    [FATAL] = "FATAL",
};

static int log(enum log_level level, const char *fmt, __builtin_va_list args) 
{
    int ret;

    if (level < sys_level) {
        return 0;
    }

    if (level > FATAL) {
        return -1;
    }
    ret = printk("[%s] ", log_level_name[level]);
    if (ret < 0) {
        return ret;
    }
    return vprintk(fmt, args);
}

int set_log_level(enum log_level level) 
{
    sys_level = level;
    return 0;
}

int log_debug(const char *fmt, ...) 
{
	__builtin_va_list args;
	__builtin_va_start(args, fmt);
	int ret = log(DEBUG, fmt, args);
	__builtin_va_end(args);
    return ret;
}

int log_info(const char *fmt, ...) 
{
	__builtin_va_list args;
	__builtin_va_start(args, fmt);
	int ret = log(INFO, fmt, args);
	__builtin_va_end(args);
    return ret;
}

int log_error(const char *fmt, ...) 
{
	__builtin_va_list args;
	__builtin_va_start(args, fmt);
	int ret = log(ERROR, fmt, args);
	__builtin_va_end(args);
    return ret;
}

int log_fatal(const char *fmt, ...) 
{
    int ret;
    u8 color, color_fatal;
	__builtin_va_list args;

    color_fatal = 0x4;
    tty_color(tty_current, TTY_OP_GET, &color);
    tty_color(tty_current, TTY_OP_SET, &color_fatal); 

	__builtin_va_start(args, fmt);
	ret = log(FATAL, fmt, args);
	__builtin_va_end(args);

    tty_color(tty_current, TTY_OP_SET, &color);
    return ret;
}