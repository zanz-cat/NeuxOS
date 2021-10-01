#ifndef __KERNEL_MM_H__
#define __KERNEL_MM_H__

#include <stdint.h>

#include <config.h>

struct page_entry {
    uint32_t present:1;
    uint32_t rw:1;
    uint32_t us:1; // 1 - user, 0 - supervisor
    uint32_t pwt:1; // Page-level Write-Through
    uint32_t pcd:1; // Page-level Cache Disable
    uint32_t access:1;
    uint32_t dirty:1;
    uint32_t pat:1; // support Page Attribute Table
    uint32_t g:1; // global
    uint32_t avl:3;
    uint32_t index:20;
} __attribute__((packed));

#define PAGE_ENTRY_COUNT (PAGE_SIZE/sizeof(struct page_entry))

struct page_entry *alloc_user_page(void);
void free_user_page(struct page_entry *table);
void mm_setup(void);
void mm_report(void);

#endif