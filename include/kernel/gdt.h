#ifndef __GDT_H__
#define __GDT_H__

#include <type.h>
#include <kernel/protect.h>

void gdt_init();
int install_tss(struct tss *ptss);
int uninstall_tss(u16 sel);
int install_ldt(void *ldt, u16 size);
int uninstall_ldt(u16 sel);

#endif