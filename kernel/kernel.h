#ifndef __KERNEL_H__
#define __KERNEL_H__

/* GDT 和 IDT 中描述符的个数 */
#define GDT_SIZE    128
#define IDT_SIZE    256

/* 权限 */
#define PRIVILEGE_KRNL  0
#define PRIVILEGE_TASK  1
#define PRIVILEGE_USER  3

/* 8253/8254 PIT */
#define TIMER0          0x40
#define TIMER_MODE      0x43
#define RATE_GENERATOR  0x34
#define TIMER_FREQ      1193182L
#define HZ              100

#endif