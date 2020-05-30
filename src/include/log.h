#ifndef __LOG_H__
#define __LOG_H__

enum LEVEL {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

int log(enum LEVEL level, const char *fmt, ...);
int log_debug(const char *fmt, ...);
int log_info(const char *fmt, ...);
int log_warn(const char *fmt, ...);
int log_error(const char *fmt, ...);
int log_fatal(const char *fmt, ...);

#endif