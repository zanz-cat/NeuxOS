#ifndef __LOG_H__
#define __LOG_H__

typedef enum {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
} LOG_LEVEL;

int set_log_level(LOG_LEVEL level);
int log_debug(const char *fmt, ...);
int log_info(const char *fmt, ...);
int log_warn(const char *fmt, ...);
int log_error(const char *fmt, ...);
int log_fatal(const char *fmt, ...);

#endif