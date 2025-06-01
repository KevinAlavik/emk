/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/paging.h>
#include <mm/pmm.h>
#include <lib/string.h>
#include <sys/kpanic.h>
#include <boot/emk.h>
#include <util/log.h>
#include <util/align.h>

uint64_t *kernel_pagemap = NULL;
extern char __limine_requests_start[];
extern char __limine_requests_end[];
extern char __text_start[];
extern char __text_end[];
extern char __rodata_start[];
extern char __rodata_end[];
extern char __data_start[];
extern char __data_end[];

#define PRINT_SECTION(name, start, end) log_early("section=%s, start=0x%.16llx, end=0x%.16llx, size=%lld", name, (uint64_t)start, (uint64_t)end, (uint64_t)(end - start))

/* Helper: Calculate page table index for a virtual address */
static inline uint64_t page_index(uint64_t virt, uint64_t shift)
{
    return (virt >> shift) & PAGE_INDEX_MASK;
}

/* Helper: Get page table entry, return NULL if not present */
static inline uint64_t *get_table(uint64_t *table, uint64_t index)
{
    if (!table || !(table[index] & VMM_PRESENT))
    {
        return NULL;
    }
    return (uint64_t *)HIGHER_HALF(table[index] & PAGE_MASK);
}

/* Helper: Get or allocate a page table */
static inline uint64_t *get_or_alloc_table(uint64_t *table, uint64_t index, uint64_t flags)
{
    if (!table)
    {
        return NULL;
    }
    if (!(table[index] & VMM_PRESENT))
    {
        uint64_t *new_table = palloc(1, true);
        if (!new_table || !IS_PAGE_ALIGNED((uint64_t)new_table))
        {
            return NULL;
        }
        memset(new_table, 0, PAGE_SIZE);
        table[index] = (uint64_t)PHYSICAL(new_table) | 0b111;
    }
    table[index] |= flags & 0xFF;
    return (uint64_t *)HIGHER_HALF(table[index] & PAGE_MASK);
}

/* Translate virtual to physical address */
uint64_t virt_to_phys(uint64_t *pagemap, uint64_t virt)
{
    if (!pagemap)
    {
        return 0;
    }

    uint64_t pml4_idx = page_index(virt, PML4_SHIFT);
    uint64_t *pml3 = get_table(pagemap, pml4_idx);
    if (!pml3)
    {
        return 0;
    }

    uint64_t pml3_idx = page_index(virt, PML3_SHIFT);
    uint64_t *pml2 = get_table(pml3, pml3_idx);
    if (!pml2)
    {
        return 0;
    }

    uint64_t pml2_idx = page_index(virt, PML2_SHIFT);
    uint64_t *pml1 = get_table(pml2, pml2_idx);
    if (!pml1)
    {
        return 0;
    }

    uint64_t pml1_idx = page_index(virt, PML1_SHIFT);
    if (!(pml1[pml1_idx] & VMM_PRESENT))
    {
        return 0;
    }

    return pml1[pml1_idx] & PAGE_MASK;
}

/* Set active pagemap (load CR3) */
void pmset(uint64_t *pagemap)
{
    if (!pagemap || !IS_PAGE_ALIGNED((uint64_t)PHYSICAL(pagemap)))
    {
        kpanic(NULL, "Invalid pagemap");
    }
    __asm__ volatile("movq %0, %%cr3" ::"r"((uint64_t)PHYSICAL(pagemap)) : "memory");
}

/* Get current pagemap from CR3 */
uint64_t *pmget(void)
{
    uint64_t cr3;
    __asm__ volatile("movq %%cr3, %0" : "=r"(cr3));
    return (uint64_t *)HIGHER_HALF(cr3);
}

/* Map virtual to physical address (large 2M pages) */
#if LARGE_PAGES
bool _supports_large_pages()
{
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1));
    return (edx & (1 << 3)); // PSE (bit 3) = 2M page support
}

int vmap_large(uint64_t *pagemap, uint64_t virt, uint64_t phys, uint64_t flags)
{
    if (!pagemap || (virt & (PAGE_SIZE_2M - 1)) || (phys & (PAGE_SIZE_2M - 1)))
        return -1;

    if (!_supports_large_pages())
    {
        log_early("error: vmap_large() is unsupported: 2MB pages not supported!");
        return -1;
    }

    uint64_t pml4_idx = page_index(virt, PML4_SHIFT);
    uint64_t pml3_idx = page_index(virt, PML3_SHIFT);
    uint64_t pml2_idx = page_index(virt, PML2_SHIFT);

    uint64_t *pml3 = get_or_alloc_table(pagemap, pml4_idx, flags);
    if (!pml3)
        return -1;

    uint64_t *pml2 = get_or_alloc_table(pml3, pml3_idx, flags);
    if (!pml2)
        return -1;

    // Set large page (PS bit = 1 << 7)
    pml2[pml2_idx] = phys | flags | (1ULL << 7);
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
    return 0;
}
#else  // LARGE_PAGES
bool _supports_large_pages()
{
    log_early("warning: Large pages are disabled in the kernel");
    return false;
}

int vmap_large(uint64_t *, uint64_t, uint64_t, uint64_t)
{
    return -1;
}
#endif // LARGE_PAGES

/* Map virtual to physical address */
int vmap(uint64_t *pagemap, uint64_t virt, uint64_t phys, uint64_t flags)
{
    if (!pagemap)
    {
        return -1;
    }

    uint64_t pml4_idx = page_index(virt, PML4_SHIFT);
    uint64_t pml3_idx = page_index(virt, PML3_SHIFT);
    uint64_t pml2_idx = page_index(virt, PML2_SHIFT);
    uint64_t pml1_idx = page_index(virt, PML1_SHIFT);

    uint64_t *pml3 = get_or_alloc_table(pagemap, pml4_idx, flags);
    if (!pml3)
    {
        return -1;
    }

    uint64_t *pml2 = get_or_alloc_table(pml3, pml3_idx, flags);
    if (!pml2)
    {
        return -1;
    }

    uint64_t *pml1 = get_or_alloc_table(pml2, pml2_idx, flags);
    if (!pml1)
    {
        return -1;
    }

    pml1[pml1_idx] = phys | flags;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
    return 0;
}

/* Unmap virtual address */
int vunmap(uint64_t *pagemap, uint64_t virt)
{
    if (!pagemap || (virt & (PAGE_SIZE - 1)))
    {
        return -1;
    }

    uint64_t pml4_idx = page_index(virt, PML4_SHIFT);
    uint64_t *pml3 = get_table(pagemap, pml4_idx);
    if (!pml3)
    {
        return 0;
    }

    uint64_t pml3_idx = page_index(virt, PML3_SHIFT);
    uint64_t *pml2 = get_table(pml3, pml3_idx);
    if (!pml2)
    {
        return 0;
    }

    uint64_t pml2_idx = page_index(virt, PML2_SHIFT);
    uint64_t *pml1 = get_table(pml2, pml2_idx);
    if (!pml1)
    {
        return 0;
    }

    uint64_t pml1_idx = page_index(virt, PML1_SHIFT);
    pml1[pml1_idx] = 0;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
    return 0;
}

/* Initialize kernel paging */
void paging_init(void)
{
    kernel_pagemap = palloc(1, true);
    if (!kernel_pagemap || !IS_PAGE_ALIGNED((uint64_t)kernel_pagemap))
    {
        kpanic(NULL, "Failed to allocate kernel pagemap");
    }
    memset(kernel_pagemap, 0, PAGE_SIZE);

    if (_supports_large_pages())
        log_early("Support for 2MB pages is present");

    /* Map kernel stack */
    uint64_t stack_top = ALIGN_UP(kstack_top, PAGE_SIZE);
    for (uint64_t addr = stack_top - (16 * 1024); addr < stack_top; addr += PAGE_SIZE)
    {
        uint64_t phys = (uint64_t)PHYSICAL(addr);
        vmap(kernel_pagemap, addr, phys, VMM_PRESENT | VMM_WRITE | VMM_NX);
    }

    /* Map bootloader requests */
    uint64_t breq_start = ALIGN_DOWN(__limine_requests_start, PAGE_SIZE);
    uint64_t breq_end = ALIGN_UP(__limine_requests_end, PAGE_SIZE);
    PRINT_SECTION("limine", breq_start, breq_end);
    for (uint64_t reqs = breq_start; reqs < breq_end; reqs += PAGE_SIZE)
    {
        vmap(kernel_pagemap, reqs, reqs - kvirt + kphys, VMM_PRESENT | VMM_WRITE);
    }

    /* Map kernel sections */
    uint64_t text_start = ALIGN_DOWN((uint64_t)__text_start, PAGE_SIZE);
    uint64_t text_end = ALIGN_UP((uint64_t)__text_end, PAGE_SIZE);
    PRINT_SECTION(".text", text_start, text_end);
    for (uint64_t addr = text_start; addr < text_end; addr += PAGE_SIZE)
    {
        uint64_t phys = addr - kvirt + kphys;
        vmap(kernel_pagemap, addr, phys, VMM_PRESENT);
    }

    uint64_t rodata_start = ALIGN_DOWN((uint64_t)__rodata_start, PAGE_SIZE);
    uint64_t rodata_end = ALIGN_UP((uint64_t)__rodata_end, PAGE_SIZE);
    PRINT_SECTION(".rodata", rodata_start, rodata_end);
    for (uint64_t addr = rodata_start; addr < rodata_end; addr += PAGE_SIZE)
    {
        uint64_t phys = addr - kvirt + kphys;
        vmap(kernel_pagemap, addr, phys, VMM_PRESENT | VMM_NX);
    }

    uint64_t data_start = ALIGN_DOWN((uint64_t)__data_start, PAGE_SIZE);
    uint64_t data_end = ALIGN_UP((uint64_t)__data_end, PAGE_SIZE);
    PRINT_SECTION(".data", data_start, data_end);
    for (uint64_t addr = data_start; addr < data_end; addr += PAGE_SIZE)
    {
        uint64_t phys = addr - kvirt + kphys;
        vmap(kernel_pagemap, addr, phys, VMM_PRESENT | VMM_WRITE | VMM_NX);
    }

    /* Map HHDM */
    for (uint64_t gb4 = 0; gb4 < 0x100000000; gb4 += PAGE_SIZE)
    {
        vmap(kernel_pagemap, (uint64_t)HIGHER_HALF(gb4), gb4, VMM_PRESENT | VMM_WRITE);
    }

    /* Map physical memory with large pages when possible */
    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        if (!memmap->entries[i]->base || !memmap->entries[i]->length)
        {
            continue;
        }

        uint64_t base = ALIGN_DOWN(memmap->entries[i]->base, PAGE_SIZE);
        uint64_t end = ALIGN_UP(memmap->entries[i]->base + memmap->entries[i]->length, PAGE_SIZE);

        for (uint64_t addr = base; addr < end;)
        {
            uint64_t virt = (uint64_t)HIGHER_HALF(addr);

            // Try to map a 2MB page if both physical and virtual addresses are aligned
            if (((addr & (PAGE_SIZE_2M - 1)) == 0) && ((virt & (PAGE_SIZE_2M - 1)) == 0) &&
                (end - addr) >= PAGE_SIZE_2M)
            {
                if (vmap_large(kernel_pagemap, virt, addr, VMM_PRESENT | VMM_WRITE | VMM_NX) == 0)
                {
                    addr += PAGE_SIZE_2M;
                    continue;
                }
            }

            // Fallback to normal 4KB page
            vmap(kernel_pagemap, virt, addr, VMM_PRESENT | VMM_WRITE | VMM_NX);
            addr += PAGE_SIZE;
        }
    }

    pmset(kernel_pagemap);
}