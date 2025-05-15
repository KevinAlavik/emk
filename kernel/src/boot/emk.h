/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef EMK_H
#define EMK_H

#include <boot/limine.h>
#include <stdint.h>
#include <mm/vmm.h>
#if FLANTERM_SUPPORT
#include <flanterm/flanterm.h>
#endif // FLANTERM_SUPPORT

extern uint64_t hhdm_offset;
extern struct limine_memmap_response *memmap;
extern uint64_t kvirt;
extern uint64_t kphys;
extern uint64_t kstack_top;
extern vctx_t *kvm_ctx;
extern struct limine_rsdp_response *rsdp_response;

#define HIGHER_HALF(ptr) ((void *)((uint64_t)(ptr) < hhdm_offset ? (uint64_t)(ptr) + hhdm_offset : (uint64_t)(ptr)))
#define PHYSICAL(ptr) ((void *)((uint64_t)(ptr) >= hhdm_offset ? (uint64_t)(ptr) - hhdm_offset : (uint64_t)(ptr)))

#define BIT(x) (1ULL << (x))

#ifndef FLANTERM_SUPPORT
#define FLANTERM_SUPPORT 0
#endif // FLANTERM_SUPPORT

#ifndef BUILD_MODE
#define BUILD_MODE "unknown"
#endif // BUILD_MODE

#if FLANTERM_SUPPORT
extern struct flanterm_context *ft_ctx;
#endif // FLANTERM_SUPPORT

#endif // EMK_H