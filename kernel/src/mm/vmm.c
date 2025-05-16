/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <util/log.h>
#include <util/align.h>
#include <boot/emk.h>
#include <arch/paging.h>
#include <lib/string.h>

vctx_t *vinit(uint64_t *pm, uint64_t start)
{
    vctx_t *ctx = (vctx_t *)palloc(1, true);
    if (!ctx)
        return NULL;
    memset(ctx, 0, sizeof(vctx_t));
    ctx->root = (vregion_t *)palloc(1, true);
    if (!ctx->root)
    {
        pfree(ctx, 1);
        return NULL;
    }

    ctx->pagemap = pm;
    ctx->root->start = start;
    ctx->root->pages = 0;
    return ctx;
}

void vdestroy(vctx_t *ctx)
{
    if (ctx->root == NULL || ctx->pagemap == NULL)
        return;

    vregion_t *region = ctx->root;
    while (region != NULL)
    {
        vregion_t *next = region->next;
        pfree(region, 1);
        region = next;
    }
    pfree(ctx, 1);
}

void *valloc(vctx_t *ctx, size_t pages, uint64_t flags)
{
    if (ctx == NULL || ctx->root == NULL || ctx->pagemap == NULL)
        return NULL;

    vregion_t *region = ctx->root;
    vregion_t *new = NULL;
    vregion_t *last = ctx->root;

    while (region)
    {
        if (region->next == NULL || region->start + region->pages < region->next->start)
        {
            new = (vregion_t *)palloc(1, true);
            if (!new)
                return NULL;

            memset(new, 0, sizeof(vregion_t));
            new->pages = pages;
            new->flags = VFLAGS_TO_PFLAGS(flags);
            new->start = region->start + (region->pages * PAGE_SIZE);
            new->next = region->next;
            new->prev = region;
            region->next = new;
            for (uint64_t i = 0; i < pages; i++)
            {
                uint64_t page = (uint64_t)palloc(1, false);
                if (page == 0)
                    return NULL;

                vmap(ctx->pagemap, new->start + i * PAGE_SIZE, page, new->flags);
            }
            return (void *)new->start;
        }
        region = region->next;
    }

    new = (vregion_t *)palloc(1, true);
    if (!new)
        return NULL;

    memset(new, 0, sizeof(vregion_t));
    last->next = new;
    new->prev = last;
    new->start = last->start + (last->pages * PAGE_SIZE);
    new->pages = pages;
    new->flags = VFLAGS_TO_PFLAGS(flags);
    new->next = NULL;

    for (uint64_t i = 0; i < pages; i++)
    {
        uint64_t page = (uint64_t)palloc(1, false);
        if (page == 0)
            return NULL;

        vmap(ctx->pagemap, new->start + i * PAGE_SIZE, page, new->flags);
    }
    return (void *)new->start;
}

void *vallocat(vctx_t *ctx, size_t pages, uint64_t flags, uint64_t phys)
{
    if (ctx == NULL || ctx->root == NULL || ctx->pagemap == NULL)
        return NULL;

    vregion_t *region = ctx->root;
    vregion_t *new = NULL;
    vregion_t *last = ctx->root;

    while (region)
    {
        if (region->next == NULL || region->start + region->pages < region->next->start)
        {
            new = (vregion_t *)palloc(1, true);
            if (!new)
                return NULL;

            memset(new, 0, sizeof(vregion_t));
            new->pages = pages;
            new->flags = VFLAGS_TO_PFLAGS(flags);
            new->start = region->start + (region->pages * PAGE_SIZE);
            new->next = region->next;
            new->prev = region;
            region->next = new;
            for (uint64_t i = 0; i < pages; i++)
            {
                uint64_t page = phys + i * PAGE_SIZE;
                if (page == 0)
                    return NULL;

                vmap(ctx->pagemap, new->start + i * PAGE_SIZE, page, new->flags);
            }
            return (void *)new->start;
        }
        region = region->next;
    }

    new = (vregion_t *)palloc(1, true);
    if (!new)
        return NULL;

    memset(new, 0, sizeof(vregion_t));
    last->next = new;
    new->prev = last;
    new->start = last->start + (last->pages * PAGE_SIZE);
    new->pages = pages;
    new->flags = VFLAGS_TO_PFLAGS(flags);
    new->next = NULL;

    for (uint64_t i = 0; i < pages; i++)
    {
        uint64_t page = phys + i * PAGE_SIZE;
        if (page == 0)
            return NULL;

        vmap(ctx->pagemap, new->start + i * PAGE_SIZE, page, new->flags);
    }
    return (void *)new->start;
}

void vfree(vctx_t *ctx, void *ptr)
{
    if (ctx == NULL)
        return;

    vregion_t *region = ctx->root;
    while (region != NULL)
    {
        if (region->start == (uint64_t)ptr)
        {
            break;
        }
        region = region->next;
    }

    if (region == NULL)
        return;

    vregion_t *prev = region->prev;
    vregion_t *next = region->next;

    for (uint64_t i = 0; i < region->pages; i++)
    {
        uint64_t virt = region->start + i * PAGE_SIZE;
        uint64_t phys = virt_to_phys(kernel_pagemap, virt);

        if (phys != 0)
        {
            pfree((void *)phys, 1);
            vunmap(ctx->pagemap, virt);
        }
    }

    if (prev != NULL)
        prev->next = next;

    if (next != NULL)
        next->prev = prev;

    if (region == ctx->root)
        ctx->root = next;

    pfree(region, 1);
}