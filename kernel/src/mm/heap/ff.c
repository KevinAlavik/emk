/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <boot/emk.h>
#include <mm/heap.h>
#include <mm/heap/ff.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/kpanic.h>
#include <util/align.h>
#include <util/log.h>

void* pool;
block_t* freelist = NULL;

void heap_init() {
    pool = valloc(kvm_ctx, FF_POOL_SIZE, VALLOC_RW);
    if (!pool)
        kpanic(NULL, "Failed to alloc memory for the heap pool");
    freelist = (block_t*)pool;
    freelist->size = (FF_POOL_SIZE * PAGE_SIZE) - sizeof(block_t);
    freelist->next = NULL;
    log_early("Initialized heap with a pool of %d pages (~%dMB)", FF_POOL_SIZE,
              DIV_ROUND_UP(FF_POOL_SIZE * PAGE_SIZE, 1024 * 1024));
}

void* kmalloc(size_t size) {
    if (size == 0)
        return NULL;

    size = ALIGN_UP(size, 8);

    block_t* prev = NULL;
    block_t* cur = freelist;

    while (cur) {
        if (cur->size >= size) {
            size_t tot_size = sizeof(block_t) + size;

            if (cur->size >= ALIGN_UP(tot_size + sizeof(block_t), 8)) {
                /* do splitting */
                block_t* new_block = (block_t*)((uint8_t*)cur + tot_size);
                new_block->size = cur->size - tot_size;
                new_block->next = cur->next;

                cur->size = size;

                if (prev)
                    prev->next = new_block;
                else
                    freelist = new_block;
            } else {
                /* use entire block */
                if (prev)
                    prev->next = cur->next;
                else
                    freelist = cur->next;
            }

            return (void*)((uint8_t*)cur + sizeof(block_t));
        }
        prev = cur;
        cur = cur->next;
    }

    return NULL;
}

void kfree(void* ptr) {
    if (!ptr)
        return;

    block_t* block = (block_t*)((uint8_t*)ptr - sizeof(block_t));

    /* insert block sorted by address in freelist */
    block_t** cur = &freelist;
    while (*cur && *cur < block) {
        cur = &(*cur)->next;
    }

    block->next = *cur;
    *cur = block;

    /* coalesce with next block if adjacent */
    if (block->next && (uint8_t*)block + sizeof(block_t) + block->size ==
                           (uint8_t*)block->next) {
        block->size += sizeof(block_t) + block->next->size;
        block->next = block->next->next;
    }

    /* coalesce with previous block if adjacent */
    if (cur != &freelist) {
        block_t* prev = freelist;
        while (prev->next != block)
            prev = prev->next;
        if ((uint8_t*)prev + sizeof(block_t) + prev->size == (uint8_t*)block) {
            prev->size += sizeof(block_t) + block->size;
            prev->next = block->next;
        }
    }
}
