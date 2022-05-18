#include <string.h>

#include <kernel/log.h>
#include <arch/x86.h>
#include <lib/misc.h>

#include "page.h"

#define MAX_PAGES_COUNT ((0x100000000) >> PAGE_SHIFT)

static uint8_t _page_users[MAX_PAGES_COUNT];

uint8_t page_users(uint32_t page)
{
    return _page_users[page >> PAGE_SHIFT];
}

uint32_t alloc_page(uint8_t flags)
{
    uint32_t i;

    if (flags == PGF_HIGHMEM) {
        // TODO
        return 0xffffffff;
    }
    for (i = 0; i < MAX_PAGES_COUNT; i++) {
        if (_page_users[i] != 0) {
            continue;
        }
        _page_users[i]++;
        return i << PAGE_SHIFT;
    }
    return 0xffffffff;
}

uint32_t obtain_page(uint32_t page)
{
    _page_users[page >> PAGE_SHIFT]++;
    return page;
}

void free_page(uint32_t page)
{
    _page_users[page >> PAGE_SHIFT]--;
}

void init_page(void)
{
    uint32_t i, j, cnt, base, addr;
    uint32_t pages = 0;

    memset(_page_users, 0xff, MAX_PAGES_COUNT);
    for (i = 0; i < SHARE_DATA()->ards_cnt; i++) {
        if (SHARE_DATA()->ards[i].type != MEM_TYPE_AVL) {
            continue;
        }
        if (SHARE_DATA()->ards[i].base_addr_low % PAGE_SIZE != 0) {
            log_error("memory block(%d) is not aligned on page size\n", i);
            continue;
        }
        base = SHARE_DATA()->ards[i].base_addr_low >> PAGE_SHIFT;
        cnt = SHARE_DATA()->ards[i].len_low >> PAGE_SHIFT;
        for (j = 0; j < cnt; j++) {
            addr = (base + j) << PAGE_SHIFT;
            if (addr < ALIGN_CEIL(SHARE_DATA()->kernel_end, PAGE_SIZE) + CONFIG_KERNEL_RES_PAGES*PAGE_SIZE) {
                // <1MB and kernel memory not available
                continue;
            }
            _page_users[base + j] = 0;
            pages++;
        }
    }
    log_info("%u free pages\n", pages);
}

void page_report(void)
{
#include <kernel/printk.h>
    uint32_t i;
    size_t total;
    size_t free = 0;

    // printk("page        users\n");
    for (i = 0; i < MAX_PAGES_COUNT; i++) {
        if (_page_users[i] == 0xff) {
            continue;
        }
        if (_page_users[i] == 0) {
            free += PAGE_SIZE;
            continue;
        }
        // printk("0x%08x  %u\n", i << PAGE_SHIFT, _page_users[i]);
    }
    total = get_total_memory();
    printk("%u total, %u used, %u free\n", total, total - free, free);
}