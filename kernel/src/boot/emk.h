/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef EMK_H
#define EMK_H

#include <boot/limine.h>
#include <stdint.h>
#include <mm/vmm.h>

extern uint64_t hhdm_offset;
extern struct limine_memmap_response *memmap;
extern uint64_t kvirt;
extern uint64_t kphys;
extern uint64_t kstack_top;
extern vctx_t *kvm_ctx;

#define HIGHER_HALF(ptr) ((void *)((uint64_t)(ptr) < hhdm_offset ? (uint64_t)(ptr) + hhdm_offset : (uint64_t)(ptr)))
#define PHYSICAL(ptr) ((void *)((uint64_t)(ptr) >= hhdm_offset ? (uint64_t)(ptr) - hhdm_offset : (uint64_t)(ptr)))

#define BIT(x) (1ULL << (x))

#endif // EMK_H