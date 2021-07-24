#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <type.h>

u32 sleep(u32 sec);
u32 msleep(u32 msec);
u32 get_ticks();
void write(char *buf, int len);

#endif