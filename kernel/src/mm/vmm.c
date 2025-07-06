/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/paging.h>
#include <boot/emk.h>
#include <lib/string.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <util/align.h>
#include <util/log.h>

vctx_t* vinit(uint64_t* pm, uint64_t start) {
    vctx_t* ctx = (vctx_t*)palloc(1, true);
    if (!ctx)
        return NULL;
    memset(ctx, 0, sizeof(vctx_t));
    ctx->root = (vregion_t*)palloc(1, true);
    if (!ctx->root) {
        pfree(ctx, 1);
        return NULL;
    }

    ctx->pagemap = pm;
    ctx->root->start = start;
    ctx->root->pages = 0;
    return ctx;
}

void vdestroy(vctx_t* ctx) {
    if (ctx->root == NULL || ctx->pagemap == NULL)
        return;

    vregion_t* region = ctx->root;
    while (region != NULL) {
        vregion_t* next = region->next;
        pfree(region, 1);
        region = next;
    }
    pfree(ctx, 1);
}

void* valloc(vctx_t* ctx, size_t pages, uint64_t flags) {
    if (ctx == NULL || ctx->root == NULL || ctx->pagemap == NULL)
        return NULL;

    vregion_t* region = ctx->root;
    vregion_t* new = NULL;
    vregion_t* last = ctx->root;

    while (region) {
        if (region->next == NULL ||
            region->start + (region->pages * PAGE_SIZE) < region->next->start) {
            new = (vregion_t*)palloc(1, true);
            if (!new)
                return NULL;

            memset(new, 0, sizeof(vregion_t));
            new->pages = pages;
            new->flags = VFLAGS_TO_PFLAGS(flags);
            new->start = region->start + (region->pages * PAGE_SIZE);
            new->next = region->next;
            new->prev = region;
            region->next = new;
            for (uint64_t i = 0; i < pages; i++) {
                uint64_t page = (uint64_t)palloc(1, false);
                if (page == 0)
                    return NULL;

                vmap(ctx->pagemap, new->start + (i * PAGE_SIZE), page,
                     new->flags);
            }
            return (void*)new->start;
        }
        region = region->next;
    }

    new = (vregion_t*)palloc(1, true);
    if (!new)
        return NULL;

    memset(new, 0, sizeof(vregion_t));
    last->next = new;
    new->prev = last;
    new->start = last->start + (last->pages * PAGE_SIZE);
    new->pages = pages;
    new->flags = VFLAGS_TO_PFLAGS(flags);
    new->next = NULL;

    for (uint64_t i = 0; i < pages; i++) {
        uint64_t page = (uint64_t)palloc(1, false);
        if (page == 0)
            return NULL;

        vmap(ctx->pagemap, new->start + (i * PAGE_SIZE), page, new->flags);
    }
    return (void*)new->start;
}

void* vallocat(vctx_t* ctx, size_t pages, uint64_t flags, uint64_t phys) {
    if (ctx == NULL || ctx->root == NULL || ctx->pagemap == NULL)
        return NULL;

    vregion_t* region = ctx->root;
    vregion_t* new = NULL;
    vregion_t* last = ctx->root;

    phys = ALIGN_DOWN(phys, PAGE_SIZE);

    while (region) {
        if (region->next == NULL ||
            region->start + (pages * PAGE_SIZE) < region->next->start) {
            new = (vregion_t*)palloc(1, true);
            if (!new)
                return NULL;

            memset(new, 0, sizeof(vregion_t));
            new->pages = pages;
            new->flags = VFLAGS_TO_PFLAGS(flags);
            new->start = region->start + (region->pages * PAGE_SIZE);
            new->next = region->next;
            new->prev = region;
            region->next = new;
            for (uint64_t i = 0; i < pages; i++) {
                uint64_t page = phys + (i * PAGE_SIZE);
                if (page == 0)
                    return NULL;

                vmap(ctx->pagemap, new->start + (i * PAGE_SIZE), page,
                     new->flags);
            }
            return (void*)new->start;
        }
        region = region->next;
    }

    new = (vregion_t*)palloc(1, true);
    if (!new)
        return NULL;

    memset(new, 0, sizeof(vregion_t));
    last->next = new;
    new->prev = last;
    new->start = last->start + (last->pages * PAGE_SIZE);
    new->pages = pages;
    new->flags = VFLAGS_TO_PFLAGS(flags);
    new->next = NULL;

    for (uint64_t i = 0; i < pages; i++) {
        uint64_t page = phys + (i * PAGE_SIZE);
        if (page == 0)
            return NULL;

        vmap(ctx->pagemap, new->start + (i * PAGE_SIZE), page, new->flags);
    }
    return (void*)new->start;
}

void* vadd(vctx_t* ctx, uint64_t vaddr, uint64_t paddr, size_t pages,
           uint64_t flags) {
    if (ctx == NULL || ctx->root == NULL || ctx->pagemap == NULL)
        return NULL;

    vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);
    paddr = ALIGN_DOWN(paddr, PAGE_SIZE);

    uint64_t vend = vaddr + pages * PAGE_SIZE;

    vregion_t* region = ctx->root;
    while (region) {
        uint64_t rstart = region->start;
        uint64_t rend = region->start + region->pages * PAGE_SIZE;

        if ((vaddr >= rstart && vaddr < rend) ||
            (vend > rstart && vend <= rend) ||
            (vaddr <= rstart && vend >= rend)) {
            log("warning: vadd: overlapping region at 0x%lx", vaddr);
            return NULL;
        }
        region = region->next;
    }

    vregion_t* new = (vregion_t*)palloc(1, true);
    if (!new)
        return NULL;

    memset(new, 0, sizeof(vregion_t));
    new->start = vaddr;
    new->pages = pages;
    new->flags = VFLAGS_TO_PFLAGS(flags);

    new->next = ctx->root;
    if (ctx->root)
        ctx->root->prev = new;
    ctx->root = new;

    for (uint64_t i = 0; i < pages; i++) {
        uint64_t vpage = vaddr + (i * PAGE_SIZE);
        uint64_t ppage = paddr + (i * PAGE_SIZE);
        vmap(ctx->pagemap, vpage, ppage, new->flags);
    }

    return (void*)vaddr;
}

void vfree(vctx_t* ctx, void* ptr) {
    if (ctx == NULL)
        return;

    vregion_t* region = ctx->root;
    while (region != NULL) {
        if (region->start == (uint64_t)ptr) {
            break;
        }
        region = region->next;
    }

    if (region == NULL)
        return;

    vregion_t* prev = region->prev;
    vregion_t* next = region->next;

    for (uint64_t i = 0; i < region->pages; i++) {
        uint64_t virt = region->start + (i * PAGE_SIZE);
        uint64_t phys = virt_to_phys(kernel_pagemap, virt);

        if (phys != 0) {
            pfree((void*)phys, 1);
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

vregion_t* vget(vctx_t* ctx, uint64_t vaddr) {
    if (ctx == NULL || ctx->root == NULL)
        return NULL;

    vregion_t* region = ctx->root;
    while (region != NULL) {
        uint64_t start = region->start;
        uint64_t end = start + (region->pages * PAGE_SIZE);

        if (vaddr >= start && vaddr < end)
            return region;

        region = region->next;
    }

    return NULL;
}

const char* vflags_to_str(uint64_t flags) {
    static char out[8];
    int i = 0;

    if (flags & VALLOC_READ)
        out[i++] = 'R';
    if (flags & VALLOC_WRITE)
        out[i++] = 'W';
    if (flags & VALLOC_EXEC)
        out[i++] = 'X';
    if (flags & VALLOC_USER)
        out[i++] = 'U';
    if (i == 0)
        out[i++] = '-';

    out[i] = '\0';
    return out;
}

const char* vpflags_to_str(uint64_t flags) {
    static char out[8];
    int i = 0;

    if (flags & VMM_PRESENT)
        out[i++] = 'P';
    if (flags & VMM_WRITE)
        out[i++] = 'W';
    if (!(flags & VMM_NX))
        out[i++] = 'X';
    if (flags & VMM_USER)
        out[i++] = 'U';
    if (i == 0)
        out[i++] = '-';

    out[i] = '\0';
    return out;
}

void vdump(vctx_t* ctx) {
    if (!ctx) {
        log("vdump: ctx is NULL");
        return;
    }

    log("vdump for context at %p", ctx);
    vregion_t* region = ctx->root;
    while (region) {
        log("  region %p: start=0x%.16lx, pages=%lu, flags=0x%lx \t(%s)",
            region, region->start, region->pages, region->flags,
            vpflags_to_str(region->flags));
        region = region->next;
    }
}
