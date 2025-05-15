/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <boot/emk.h>

#define VMM_PRESENT (1ULL << 0)
#define VMM_WRITE (1ULL << 1)
#define VMM_USER (1ULL << 2)
#define VMM_NX (1ULL << 63)

#define PAGE_MASK 0x000FFFFFFFFFF000ULL
#define PAGE_INDEX_MASK 0x1FF

#define PML1_SHIFT 12
#define PML2_SHIFT 21
#define PML3_SHIFT 30
#define PML4_SHIFT 39

extern uint64_t *kernel_pagemap;
extern uint64_t kstack_top;

void pmset(uint64_t *pagemap);
uint64_t *pmget(void);
int vmap(uint64_t *pagemap, uint64_t virt, uint64_t phys, uint64_t flags);
int vunmap(uint64_t *pagemap, uint64_t virt);
uint64_t virt_to_phys(uint64_t *pagemap, uint64_t virt);
void paging_init(void);

#endif // PAGING_H