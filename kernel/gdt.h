#ifndef __KERNEL_GDT_H__
#define __KERNEL_GDT_H__

#include <stdint.h>

#include <arch/x86.h>

void gdt_setup();
int install_tss(struct tss *ptss);
int uninstall_tss(uint16_t sel);
int install_ldt(void *ldt, uint16_t size);
int uninstall_ldt(uint16_t sel);

#endif