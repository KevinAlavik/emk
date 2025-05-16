/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>

#ifndef VPM_MIN_ADDR
#define VPM_MIN_ADDR 0x1000
#endif // VPM_MIN_ADDR

#define VALLOC_NONE 0x0
#define VALLOC_READ (1 << 0)
#define VALLOC_WRITE (1 << 1)
#define VALLOC_EXEC (1 << 2)
#define VALLOC_USER (1 << 3)

#define VALLOC_RW (VALLOC_READ | VALLOC_WRITE)
#define VALLOC_RX (VALLOC_READ | VALLOC_EXEC)
#define VALLOC_RWX (VALLOC_READ | VALLOC_WRITE | VALLOC_EXEC)

typedef struct vregion
{
    uint64_t start;
    uint64_t pages;
    uint64_t flags;
    struct vregion *next;
    struct vregion *prev;
} vregion_t;

typedef struct vctx
{
    vregion_t *root;
    uint64_t *pagemap;
    uint64_t start;
} vctx_t;

vctx_t *vinit(uint64_t *pm, uint64_t start);
void vdestroy(vctx_t *ctx);
void *valloc(vctx_t *ctx, size_t pages, uint64_t flags);
void *vallocat(vctx_t *ctx, size_t pages, uint64_t flags, uint64_t phys);
void vfree(vctx_t *ctx, void *ptr);

#endif // VMM_H