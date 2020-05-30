#ifndef __GDT_H__
#define __GDT_H__

#include "type.h"
#include "protect.h"

void init_gdt();
int install_tss(TSS *ptss);
int install_ldt(void *ldt, u16 size);

#endif