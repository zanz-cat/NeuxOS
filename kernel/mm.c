#include <stdint.h>
#include <string.h>

#include <config.h>
#include <misc/misc.h>
#include <misc/misc.h>

#include "log.h"
#include "kernel.h"
#include "printk.h"
#include "interrupt.h"
#include "sched.h"

#include "mm.h"

struct heap_block {
    size_t size;
    struct heap_block *prev;
    struct heap_block *next;
    uint8_t data[0];
} __attribute__((packed));

#define KERNEL_MEM_PERCENT 4
#define MAX_PAGES_COUNT ((0x100000000)/CONFIG_MEM_PAGE_SIZE)
#define PAGES_BITMAP_SIZE (MAX_PAGES_COUNT / 8)

#define NEXT_BLOCK(b) \
    ((typeof(b))((uint32_t)(b) + sizeof(struct heap_block) + (b)->size))

#define NEXT_BLOCK_DATA_PTR(b) ((void*)(NEXT_BLOCK(b)->data))

#define ALLOC_BLOCK(b, align) \
__extension__ ({ \
    container_of(PTR_ALIGN_CEIL(NEXT_BLOCK_DATA_PTR(b), align == 0 ? 1 : align), \
                 struct heap_block, data); \
})

static size_t total_mem;
static uint32_t heap_addr;
static uint8_t pages_bitmap[PAGES_BITMAP_SIZE];
static struct heap_block *heap;
static struct {
    struct paging_entry dir_table[PAGE_ENT_CNT];
    struct paging_entry page_table[PAGE_ENT_CNT][PAGE_ENT_CNT];
} *kernel_page_table;
static uint32_t kernel_pages_count;

static inline void pages_bitmap_set(uint32_t page_no)
{
    pages_bitmap[page_no/8] |= (1U << (page_no%8));
}

static inline void pages_bitmap_clear(uint32_t page_no)
{
    pages_bitmap[page_no/8] &= ~(1U << (page_no%8));
}

static void setup_prepare(void)
{
    uint32_t i;

    total_mem = 0;
    for (i = SHARE_DATA()->ards_cnt - 1; i >= 0; i--) {
        if (SHARE_DATA()->ards[i].type == MEM_TYPE_AVL) {
            total_mem = SHARE_DATA()->ards[i].base_addr_low + SHARE_DATA()->ards[i].len_low;
            break;
        }
    }
    log_info("total memory %u bytes\n", total_mem);

    heap_addr = SHARE_DATA()->kernel_end + CONFIG_KERNEL_VMA;
}

static void init_pages_bitmap(void)
{
    uint32_t i, j, cnt, base, addr;
    uint32_t pages = 0;

    memset(pages_bitmap, 0xff, PAGES_BITMAP_SIZE);
    for (i = 0; i < SHARE_DATA()->ards_cnt; i++) {
        if (SHARE_DATA()->ards[i].type != MEM_TYPE_AVL) {
            continue;
        }
        if (SHARE_DATA()->ards[i].base_addr_low % CONFIG_MEM_PAGE_SIZE != 0) {
            log_error("memory block(%d) is not aligned on page size\n", i);
            continue;
        }
        base = SHARE_DATA()->ards[i].base_addr_low / CONFIG_MEM_PAGE_SIZE;
        cnt = SHARE_DATA()->ards[i].len_low / CONFIG_MEM_PAGE_SIZE;
        for (j = 0; j < cnt; j++) {
            addr = (base + j) * CONFIG_MEM_PAGE_SIZE;
            if (addr < SHARE_DATA()->kernel_end) {
                // 0 ~ 1MB and kernel memory not available
                continue;
            }
            pages_bitmap_clear(base + j);
            pages++;
        }
    }
    log_info("%u free pages\n", pages);
}

struct paging_entry *mm_alloc_user_page_table(void)
{
    struct paging_entry *table = mm_malloc(CONFIG_MEM_PAGE_SIZE, CONFIG_MEM_PAGE_SIZE);
    if (table == NULL) {
        log_error("alloc paging table error\n");
        return NULL;
    }
    memset(table, 0, CONFIG_MEM_PAGE_SIZE);
    memcpy(table, kernel_page_table->dir_table, kernel_pages_count * sizeof(struct paging_entry));
    return table;
}

void mm_free_user_paging(struct paging_entry *dir_table)
{
    mm_free(dir_table);
}

static void setup_heap(void)
{
    uint32_t unused_addr = SHARE_DATA()->kernel_end + CONFIG_KERNEL_VMA;
    unused_addr = ALIGN_CEIL(unused_addr, CONFIG_MEM_PAGE_SIZE) + 
                    CONFIG_KERNEL_RES_PAGES * CONFIG_MEM_PAGE_SIZE;

    heap = (struct heap_block *)unused_addr;
    heap->size = 0;
    heap->prev = heap;
    heap->next = heap;
    pages_bitmap_set((uint32_t)heap / CONFIG_MEM_PAGE_SIZE);
}

void *mm_malloc(size_t size, uint32_t align)
{
    struct heap_block *block;
    struct heap_block *new_block;
    
    if (size == 0 || (align && !is_power_of_2(align))) {
        return NULL;
    }

    for (block = heap, new_block = ALLOC_BLOCK(block, align);
         block->next != heap && ((uint32_t)new_block->data+size > (uint32_t)block->next); 
         block = block->next, new_block = ALLOC_BLOCK(block, align));

    new_block->size = size;
    new_block->prev = block;
    new_block->next = block->next;
    block->next = new_block;
    new_block->next->prev = new_block;

    return (void *)new_block->data;
}

void mm_free(void *ptr)
{
    struct heap_block *block = container_of(ptr, struct heap_block, data);
    block->prev->next = block->next;
    block->next->prev = block->prev;
    block->prev = NULL;
    block->next = NULL;
}

static void page_fault_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
    log_error("Page Fault, term task %u\n", current->pid);
    term_task(current->pid);
    sched_task();
    asm("ljmp *(%0)"::"p"(PTR_SUB(&current->tss_sel, sizeof(uint32_t))):);
}

void mm_report(void)
{
#define MM_REPORT_TEMPLATE "%3u %010p %8u %010p %010p\n"

    struct heap_block *block;
    uint32_t i = 0;
    uint32_t used = 0;

    printk("*** Memory Allocation Report ***\n[no addr size prev next]\n");
    printk(MM_REPORT_TEMPLATE, i++, heap, heap->size, heap->prev, heap->next);
    for (block = heap->next; block != heap; block = block->next) {
        used += block->size;
        printk(MM_REPORT_TEMPLATE, i++, block, block->size, block->prev, block->next);
    }
    printk("Summary:\n total %u bytes\n used %u bytes\n free %u bytes\n usage: %u%%\n", 
            total_mem, used, total_mem - used, used * 100 / total_mem);
}

void mm_setup(void)
{
    log_info("setup memory management\n");
    log_info("page size %u bytes\n", CONFIG_MEM_PAGE_SIZE);
    setup_prepare();
    init_pages_bitmap();
    setup_heap();
    irq_register_ex_handler(IRQ_EX_PAGE_FAULT, page_fault_handler);
}