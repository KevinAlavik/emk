#ifndef EMK_H
#define EMK_H

#include <boot/limine.h>
#include <stdint.h>

extern uint64_t hhdm_offset;
extern struct limine_memmap_response *memmap;

#define HIGHER_HALF(ptr) ((void *)((uint64_t)ptr) + hhdm_offset)
#define PHYSICAL(ptr) ((void *)((uint64_t)ptr) - hhdm_offset)

#define BIT(x) (1ULL << (x))

#endif // EMK_H