#ifndef PAGING_H
#define PAGING_H

#include <boot/emk.h>

#define VMM_PRESENT BIT(0)
#define VMM_WRITE BIT(1)
#define VMM_USER BIT(2)
#define VMM_NX BIT(63)

#define PAGE_MASK 0x000FFFFFFFFFF000ULL
#define PAGE_INDEX_MASK 0x1FF

#define PML1_SHIFT 12
#define PML2_SHIFT 21
#define PML3_SHIFT 30
#define PML4_SHIFT 39

extern uint64_t *kernel_pagemap;

void pmset(uint64_t *pagemap);
uint64_t *pmget();
void vmap(uint64_t *pagemap, uint64_t virt, uint64_t phys, uint64_t flags);
void vunmap(uint64_t *pagemap, uint64_t virt);
uint64_t virt_to_phys(uint64_t *pagemap, uint64_t virt);
void paging_init();

#endif // PAGING_H