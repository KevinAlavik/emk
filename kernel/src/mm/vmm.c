/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <util/log.h>
#include <util/align.h>
#include <boot/emk.h>
#include <arch/paging.h>

vpm_ctx_t *vmm_init(uint64_t *pm)
{
    if (!pm || !IS_PAGE_ALIGNED(PHYSICAL(pm)))
        return NULL;

    vpm_ctx_t *ctx = (vpm_ctx_t *)palloc(1, true);
    if (!ctx)
        return NULL;

    ctx->pagemap = pm;
    ctx->root = NULL; /* valloc creates the root if not present */
    return ctx;
}

void *valloc(vpm_ctx_t *ctx, size_t pages, uint64_t flags)
{
    if (!ctx || !pages)
        return NULL;

    /* Allocate physical pages */
    void *phys = palloc(pages, true);
    if (!phys)
        return NULL;

    uint64_t virt = VPM_MIN_ADDR;
    vm_region_t *prev = NULL;
    vm_region_t *curr = ctx->root;

    while (curr)
    {
        uint64_t curr_end = curr->start + (curr->pages * PAGE_SIZE);
        if (virt + (pages * PAGE_SIZE) <= curr->start)
        {
            /* Found a gap */
            break;
        }
        virt = curr_end;
        prev = curr;
        curr = curr->next;
    }

    /* Map the virtual to physical pages */
    for (size_t i = 0; i < pages; i++)
    {
        uint64_t vaddr = virt + (i * PAGE_SIZE);
        uint64_t paddr = (uint64_t)phys + (i * PAGE_SIZE);
        if (vmap(ctx->pagemap, vaddr, paddr, flags) != 0)
        {
            /* Mapping failed, unmap any mapped pages and free physical memory */
            for (size_t j = 0; j < i; j++)
            {
                vunmap(ctx->pagemap, virt + (j * PAGE_SIZE));
            }
            pfree(phys, pages);
            return NULL;
        }
    }

    /* Create new region */
    vm_region_t *region = (vm_region_t *)palloc(1, true);
    if (!region)
    {
        /* Region allocation failed, clean up */
        for (size_t i = 0; i < pages; i++)
        {
            vunmap(ctx->pagemap, virt + (i * PAGE_SIZE));
        }
        pfree(phys, pages);
        return NULL;
    }

    region->start = virt;
    region->pages = pages;
    region->next = curr;

    if (prev)
    {
        prev->next = region;
    }
    else
    {
        ctx->root = region;
    }

    return (void *)virt;
}