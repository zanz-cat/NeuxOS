#ifndef __KERNEL_MM_H__
#define __KERNEL_MM_H__

#include <stdint.h>
#include <stddef.h>

#include <config.h>

struct paging_entry {
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

#define PAGE_ENT_CNT (CONFIG_MEM_PAGE_SIZE/sizeof(struct paging_entry))

void mm_setup(void);
struct paging_entry *mm_alloc_user_page_table(void);
void mm_free_user_paging(struct paging_entry *table);
void *mm_malloc(size_t size, uint32_t align);
void mm_free(void *ptr);
void mm_report(void);

#endif