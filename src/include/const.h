#ifndef __CONST_H__
#define __CONST_H__

/* GDT 和 IDT 中描述符的个数 */
#define GDT_SIZE    128
#define IDT_SIZE    256

/* 权限 */
#define PRIVILEGE_KRNL  0
#define PRIVILEGE_TASK  1
#define PRIVILEGE_USER  3

/* 8259A interrupt controller ports. */
#define INT_M_CTL     0x20 /* I/O port for interrupt controller       <Master> */
#define INT_M_CTLMASK 0x21 /* setting bits in this port disables ints <Master> */
#define INT_S_CTL     0xa0 /* I/O port for second interrupt controller<Slave>  */
#define INT_S_CTLMASK 0xa1 /* setting bits in this port disables ints <Slave>  */

#define NULL 0

#endif