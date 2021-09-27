#ifndef __KERNEL_KERNEL_H__
#define __KERNEL_KERNEL_H__

void kernel_panic(const char *fmt, ...);
void kernel_loop(void);

#endif