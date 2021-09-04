#ifndef __LIB_LOG_H__
#define __LIB_LOG_H__

enum log_level {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

int set_log_level(enum log_level level);
int log_debug(const char *fmt, ...);
int log_info(const char *fmt, ...);
int log_error(const char *fmt, ...);
int log_fatal(const char *fmt, ...);

#endif