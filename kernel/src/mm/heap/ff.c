/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <mm/heap/ff.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <boot/emk.h>
#include <util/align.h>
#include <sys/kpanic.h>

void *pool;
block_t *freelist = NULL;

void heap_init()
{
    pool = valloc(kvm_ctx, FF_POOL_SIZE, VALLOC_RW);
    if (!pool)
        kpanic(NULL, "Failed to alloc memory for the heap pool");
    freelist = (block_t *)pool;
    freelist->size = (FF_POOL_SIZE * PAGE_SIZE) - sizeof(block_t);
    freelist->next = NULL;
}

void *kmalloc(size_t size)
{
    if (size == 0)
        return NULL;

    size = ALIGN_UP(size, 8);

    block_t *prev = NULL;
    block_t *cur = freelist;

    while (cur)
    {
        if (cur->size >= size)
        {
            size_t tot_size = sizeof(block_t) + size;

            if (cur->size >= ALIGN_UP(tot_size + sizeof(block_t), 8))
            {
                /* do splitting */
                block_t *new_block = (block_t *)((uint8_t *)cur + tot_size);
                new_block->size = cur->size - tot_size;
                new_block->next = cur->next;

                cur->size = size;

                if (prev)
                    prev->next = new_block;
                else
                    freelist = new_block;
            }
            else
            {
                /* use entire block */
                if (prev)
                    prev->next = cur->next;
                else
                    freelist = cur->next;
            }

            return (void *)((uint8_t *)cur + sizeof(block_t));
        }
        prev = cur;
        cur = cur->next;
    }

    return NULL;
}

void kfree(void *ptr)
{
    if (!ptr)
        return;

    /* get block header */
    block_t *block = (block_t *)((uint8_t *)ptr - sizeof(block_t));

    /* insert freed block at head of freelist */
    block->next = freelist;
    freelist = block;
}