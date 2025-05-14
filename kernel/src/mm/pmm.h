#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 0x1000

void pmm_init();
void *pmm_request_pages(size_t pages, bool higher_half);
void pmm_release_pages(void *ptr, size_t pages);

#endif // PMM_H