#include "log.h"
#include "stdio.h"

static LOG_LEVEL log_level = INFO;

static int log(LOG_LEVEL level, const char *fmt, __builtin_va_list args) 
{
    if (level < log_level) {
        return 0;
    }
    
    switch (level){
    case DEBUG:
        puts("[DEBUG] ");
        break;
    case INFO:
        puts("[INFO] ");
        break;
    case WARN:
        puts("[WARN] ");
        break;
    case ERROR:
        puts("[ERROR] ");
        break;
    case FATAL:
        puts("[FATAL] ");
        break;
    default:
        break;
    }
	return fprintf(current_console, fmt, args);
}

int set_log_level(LOG_LEVEL level) 
{
    log_level = level;
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
	__builtin_va_list args;
	__builtin_va_start(args, fmt);
    int color = current_console->color;
    current_console->color = 0x4;
	int ret = log(FATAL, fmt, args);
    current_console->color = color;
	__builtin_va_end(args);
    return ret;
}