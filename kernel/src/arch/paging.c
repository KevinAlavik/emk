#include <arch/paging.h>
#include <mm/pmm.h>
#include <lib/string.h>
#include <sys/kpanic.h>
#include <boot/emk.h>
#include <util/log.h>
#include <util/align.h>

uint64_t *kernel_pagemap = 0;
extern char __limine_requests_start[];
extern char __limine_requests_end[];
extern char __text_start[];
extern char __text_end[];
extern char __rodata_start[];
extern char __rodata_end[];
extern char __data_start[];
extern char __data_end[];

#define PRINT_SECTION(name, start, end) log_early("section=%s, start=0x%.16llx, end=0x%.16llx, size=%d", name, start, end, end - start)

/* Helpers */
static inline uint64_t page_index(uint64_t virt, uint64_t shift)
{
    return (virt & (uint64_t)0x1ff << shift) >> shift;
}

static inline uint64_t *get_table(uint64_t *table, uint64_t index)
{
    return (uint64_t *)HIGHER_HALF(table[index] & PAGE_MASK);
}

static inline uint64_t *get_or_alloc_table(uint64_t *table, uint64_t index, uint64_t flags)
{
    if (!(table[index] & VMM_PRESENT))
    {
        uint64_t *pml = palloc(1, true);
        memset(pml, 0, PAGE_SIZE);
        table[index] = (uint64_t)PHYSICAL(pml) | 0b111;
    }
    table[index] |= flags & 0xFF;
    return (uint64_t *)HIGHER_HALF(table[index] & PAGE_MASK);
}

uint64_t virt_to_phys(uint64_t *pagemap, uint64_t virt)
{

    uint64_t pml4_idx = page_index(virt, PML4_SHIFT);
    if (!(pagemap[pml4_idx] & VMM_PRESENT))
        return 0;

    uint64_t *pml3 = get_table(pagemap, pml4_idx);
    uint64_t pml3_idx = page_index(virt, PML3_SHIFT);
    if (!(pml3[pml3_idx] & VMM_PRESENT))
        return 0;

    uint64_t *pml2 = get_table(pml3, pml3_idx);
    uint64_t pml2_idx = page_index(virt, PML2_SHIFT);
    if (!(pml2[pml2_idx] & VMM_PRESENT))
        return 0;

    uint64_t *pml1 = get_table(pml2, pml2_idx);
    uint64_t pml1_idx = page_index(virt, PML1_SHIFT);
    if (!(pml1[pml1_idx] & VMM_PRESENT))
        return 0;

    return pml1[pml1_idx] & PAGE_MASK;
}

/* Pagemap set/get */
void pmset(uint64_t *pagemap)
{
    __asm__ volatile("movq %0, %%cr3" ::"r"(PHYSICAL((uint64_t)pagemap)));
}

uint64_t *pmget()
{
    uint64_t p;
    __asm__ volatile("movq %%cr3, %0" : "=r"(p));
    return (uint64_t *)p;
}

/* Mapping and unmapping */
void vmap(uint64_t *pagemap, uint64_t virt, uint64_t phys, uint64_t flags)
{

    uint64_t pml4_idx = page_index(virt, PML4_SHIFT);
    uint64_t pml3_idx = page_index(virt, PML3_SHIFT);
    uint64_t pml2_idx = page_index(virt, PML2_SHIFT);
    uint64_t pml1_idx = page_index(virt, PML1_SHIFT);

    uint64_t *pml3 = get_or_alloc_table(pagemap, pml4_idx, flags);
    uint64_t *pml2 = get_or_alloc_table(pml3, pml3_idx, flags);
    uint64_t *pml1 = get_or_alloc_table(pml2, pml2_idx, flags);

    pml1[pml1_idx] = phys | flags;
}

void vunmap(uint64_t *pagemap, uint64_t virt)
{
    uint64_t pml4_idx = page_index(virt, PML4_SHIFT);
    if (!(pagemap[pml4_idx] & VMM_PRESENT))
        return;

    uint64_t *pml3 = get_table(pagemap, pml4_idx);
    uint64_t pml3_idx = page_index(virt, PML3_SHIFT);
    if (!(pml3[pml3_idx] & VMM_PRESENT))
        return;

    uint64_t *pml2 = get_table(pml3, pml3_idx);
    uint64_t pml2_idx = page_index(virt, PML2_SHIFT);
    if (!(pml2[pml2_idx] & VMM_PRESENT))
        return;

    uint64_t *pml1 = get_table(pml2, pml2_idx);
    uint64_t pml1_idx = page_index(virt, PML1_SHIFT);

    pml1[pml1_idx] = 0;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void paging_init()
{
    kernel_pagemap = (uint64_t *)palloc(1, true);
    if (kernel_pagemap == NULL)
    {
        kpanic(NULL, "Failed to allocate page for kernel pagemap, halting");
    }
    memset(kernel_pagemap, 0, PAGE_SIZE);

    PRINT_SECTION("text", __text_start, __text_end);
    PRINT_SECTION("rodata", __rodata_start, __rodata_end);
    PRINT_SECTION("data", __data_start, __data_end);

    kstack_top = ALIGN_UP(kstack_top, PAGE_SIZE);
    for (uint64_t stack = kstack_top - (16 * 1024); stack < kstack_top; stack += PAGE_SIZE)
    {
        vmap(kernel_pagemap, stack, (uint64_t)PHYSICAL(stack), VMM_PRESENT | VMM_WRITE | VMM_NX);
    }
    log_early("Mapped kernel stack");

    for (uint64_t reqs = ALIGN_DOWN(__limine_requests_start, PAGE_SIZE); reqs < ALIGN_UP(__limine_requests_end, PAGE_SIZE); reqs += PAGE_SIZE)
    {
        vmap(kernel_pagemap, reqs, reqs - kvirt + kphys, VMM_PRESENT | VMM_WRITE);
    }
    log_early("Mapped Limine Requests region.");

    for (uint64_t text = ALIGN_DOWN(__text_start, PAGE_SIZE); text < ALIGN_UP(__text_end, PAGE_SIZE); text += PAGE_SIZE)
    {
        vmap(kernel_pagemap, text, text - kvirt + kphys, VMM_PRESENT);
    }
    log_early("Mapped .text");

    for (uint64_t rodata = ALIGN_DOWN(__rodata_start, PAGE_SIZE); rodata < ALIGN_UP(__rodata_end, PAGE_SIZE); rodata += PAGE_SIZE)
    {
        vmap(kernel_pagemap, rodata, rodata - kvirt + kphys, VMM_PRESENT | VMM_NX);
    }
    log_early("Mapped .rodata");

    for (uint64_t data = ALIGN_DOWN(__data_start, PAGE_SIZE); data < ALIGN_UP(__data_end, PAGE_SIZE); data += PAGE_SIZE)
    {
        vmap(kernel_pagemap, data, data - kvirt + kphys, VMM_PRESENT | VMM_WRITE | VMM_NX);
    }
    log_early("Mapped .data");

    for (uint64_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];
        uint64_t base = ALIGN_DOWN(entry->base, PAGE_SIZE);
        uint64_t end = ALIGN_UP(entry->base + entry->length, PAGE_SIZE);

        for (uint64_t addr = base; addr < end; addr += PAGE_SIZE)
        {
            vmap(kernel_pagemap, (uint64_t)HIGHER_HALF(addr), addr, VMM_PRESENT | VMM_WRITE | VMM_NX);
        }
        log_early("Mapped memory map entry %d: base=0x%.16llx, length=0x%.16llx, type=%d", i, entry->base, entry->length, entry->type);
    }

    for (uint64_t gb4 = 0; gb4 < 0x100000000; gb4 += PAGE_SIZE)
    {
        vmap(kernel_pagemap, (uint64_t)HIGHER_HALF(gb4), gb4, VMM_PRESENT | VMM_WRITE);
    }
    log_early("Mapped HHDM");
    pmset(kernel_pagemap);
}