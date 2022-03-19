#include <stdbool.h>

#include <config.h>
#include <misc/misc.h>
#include <kernel/kernel.h>
#include <kernel/lock.h>
#include <kernel/memory.h>

#include "page.h"
#include "kmalloc.h"

struct heap_block {
    size_t size;
    struct heap_block *prev;
    struct heap_block *next;
    uint8_t data[0];
} __attribute__((packed));

#define BLOCK_END(block) ((uint32_t)(block)->data+(block)->size)

#define IN_SAME_PAGE(a1,a2) (((uint32_t)(a1) & PAGE_MASK) == ((uint32_t)(a2) & PAGE_MASK))

static struct heap_block *heap;
static struct sample_lock lock = SAMPLE_LOCK_INITIALIZER;

static void *search_interspace(const struct heap_block *block, size_t size, size_t align)
{
    uint32_t saddr, eaddr;

    saddr = ALIGN_CEIL(BLOCK_END(block) + sizeof(struct heap_block), align == 0 ? 1 : align);
    saddr = (uint32_t)container_of(saddr, struct heap_block, data);
    eaddr = min((saddr & PAGE_MASK) + PAGE_SIZE, (uint32_t)block->next);
    while (true) {
        if (eaddr < saddr) {
            return NULL;
        }
        if (eaddr - saddr >= sizeof(struct heap_block) + size) {
            return (void *)saddr;
        }
        if (IN_SAME_PAGE(saddr, block->next)) {
            return NULL;
        }
        if (page_users(phy_addr(eaddr)) == 0) {
            eaddr += PAGE_SIZE;
            continue;
        }
        //RESET
        for (saddr = eaddr; !IN_SAME_PAGE(saddr, block->next) &&
            page_users(phy_addr(saddr)) > 0; saddr += PAGE_SIZE);
        eaddr = IN_SAME_PAGE(saddr, block->next) ? (uint32_t)block->next : saddr + PAGE_SIZE;
    }
}

void *kmemalign(size_t alignment, size_t size)
{
    uint32_t i;
    struct heap_block *block;
    struct heap_block *new_block;

    if (size == 0 || (alignment && !is_power_of_2(alignment))) {
        return NULL;
    }

    obtain_lock(&lock);
    for (block = heap; (new_block = search_interspace(block, size, alignment)) == NULL &&
         block->next != heap; block = block->next);
 
    if (new_block == NULL) {
        release_lock(&lock);
        return NULL;
    }

    new_block->size = size;
    new_block->prev = block;
    new_block->next = block->next;
    block->next = new_block;
    new_block->next->prev = new_block;

    for (i = (uint32_t)new_block & PAGE_MASK;
         i < ALIGN_CEIL(BLOCK_END(new_block), PAGE_SIZE);
         i += PAGE_SIZE) {
        alloc_page_x(phy_addr(i));
    }
    release_lock(&lock);

    return (void *)new_block->data;
}

void *kmalloc(size_t size)
{
    return kmemalign(0, size);
}

void kfree(void *ptr)
{
    uint32_t i;
    struct heap_block *block = NULL;
    
    if (ptr == NULL) {
        return;
    }
    
    block = container_of(ptr, struct heap_block, data);
    obtain_lock(&lock);
    for (i = (uint32_t)block & PAGE_MASK;
         i < ALIGN_CEIL(BLOCK_END(block), PAGE_SIZE);
         i += PAGE_SIZE) {
        free_page(phy_addr(i));
    }
    block->prev->next = block->next;
    block->next->prev = block->prev;
    block->prev = NULL;
    block->next = NULL;
    release_lock(&lock);
}

void init_heap(void)
{
    struct heap_block *end;
    uint32_t page = alloc_page(PGF_NORMAL);
    if (page == -1) {
        kernel_panic("setup heap error, no page\n");
    }
    heap = (struct heap_block *)virt_addr(page);
    end = (void *)(min(get_total_memory(), 0xffffffff - CONFIG_KERNEL_VM_OFFSET) +
           CONFIG_KERNEL_VM_OFFSET - sizeof(struct heap_block));
    alloc_page_x((uint32_t)end & PAGE_MASK);
    heap->size = 0;
    heap->prev = end;
    heap->next = end;
    end->size = 0;
    end->prev = heap;
    end->next = heap;
}

void heap_report(void)
{
#include <kernel/printk.h>
    const struct heap_block *block;
    printk("heap blocks:\n");
    for (block = heap; block->next != heap; block = block->next) {
        printk("%p ~ %p\n", block, BLOCK_END(block));
    }
}