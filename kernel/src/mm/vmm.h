#ifndef VMM_H
#define VMM_H

#include <stdint.h>

typedef struct vm_region
{
    uint64_t start;
    uint64_t pages;
    struct vm_region *next;
    /* TOOD: Maybe store flags */
} vm_region_t;

typedef struct vma_ctx
{
    vm_region_t *root;
    uint64_t *pagemap;
} vma_ctx_t;

void vmm_init();

#endif // VMM_H