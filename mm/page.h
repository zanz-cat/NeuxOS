#ifndef __MM_PAGE_H__
#define __MM_PAGE_H__

#include <stdint.h>

#define PGF_NORMAL 0x0
#define PGF_HIGHMEM 0x1

uint32_t alloc_page(uint8_t flags);
uint32_t alloc_page_x(uint32_t page);
void free_page(uint32_t page);
uint8_t page_users(uint32_t page);
void init_page(void);
void page_report(void);

#endif