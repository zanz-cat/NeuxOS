#ifndef __KERNEL_DESC_H__
#define __KERNEL_DESC_H__

#include <stdint.h>

#include <arch/x86.h>

void descriptor_setup();
int install_tss(struct tss *tss, uint8_t priv);
int uninstall_tss(uint16_t sel);
struct descriptor *get_descriptor(uint16_t sel);

#endif