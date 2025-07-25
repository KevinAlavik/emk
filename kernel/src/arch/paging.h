/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef PAGING_H
#define PAGING_H

#include <boot/emk.h>
#include <mm/vmm.h>
#include <stdint.h>

// Macro to convert VALLOC_* flags to VMM_* page table flags
#define VFLAGS_TO_PFLAGS(flags)                                                \
    (VMM_PRESENT | (((flags) & VALLOC_WRITE) ? VMM_WRITE : 0) |                \
     (((flags) & VALLOC_USER) ? VMM_USER : 0) |                                \
     (((flags) & VALLOC_EXEC) ? 0 : VMM_NX))

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

extern uint64_t* kernel_pagemap;
extern uint64_t kstack_top;

void pmset(uint64_t* pagemap);
uint64_t* pmget(void);
uint64_t* pmnew(void);
int vmap(uint64_t* pagemap, uint64_t virt, uint64_t phys, uint64_t flags);
int vunmap(uint64_t* pagemap, uint64_t virt);
uint64_t virt_to_phys(uint64_t* pagemap, uint64_t virt);
void paging_init(void);

#endif // PAGING_H
