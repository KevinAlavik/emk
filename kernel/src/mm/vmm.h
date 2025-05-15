/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>

#ifndef VPM_MIN_ADDR
#define VPM_MIN_ADDR 0x1000
#endif // VPM_MIN_ADDR

typedef struct vm_region
{
    uint64_t start;
    uint64_t pages;
    struct vm_region *next;
    /* TOOD: Maybe store flags */
} vm_region_t;

typedef struct vpm_ctx
{
    vm_region_t *root;
    uint64_t *pagemap;
} vpm_ctx_t;

vpm_ctx_t *vmm_init(uint64_t *pm);
void *valloc(vpm_ctx_t *ctx, size_t pages, uint64_t flags);

#endif // VMM_H