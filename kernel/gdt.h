#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>
#include "protect.h"

void gdt_init();
int install_tss(struct tss *ptss);
int uninstall_tss(uint16_t sel);
int install_ldt(void *ldt, uint16_t size);
int uninstall_ldt(uint16_t sel);

#endif