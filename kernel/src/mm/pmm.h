/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef PMM_H
#define PMM_H

#include <stdbool.h>
#include <stddef.h>

#define PAGE_SIZE 0x1000
#define PAGE_SIZE_2M (2 * 1024 * 1024)

void pmm_init();
void* palloc(size_t pages, bool higher_half);
void pfree(void* ptr, size_t pages);

#endif // PMM_H