/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <boot/limine.h>
#include <boot/emk.h>
#include <arch/cpu.h>
#include <arch/io.h>
#include <dev/serial.h>
#include <util/kprintf.h>
#include <util/log.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <sys/kpanic.h>
#include <mm/pmm.h>
#include <arch/paging.h>
#include <mm/vmm.h>
#include <mm/heap.h>
#if FLANTERM_SUPPORT
#include <flanterm/flanterm.h>
#include <flanterm/backends/fb.h>
#endif // FLANTERM_SUPPORT

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);
__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0};
__attribute__((used, section(".limine_requests"))) volatile struct limine_executable_address_request kernel_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .response = 0};
#if FLANTERM_SUPPORT
__attribute__((used, section(".limine_requests"))) volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .response = 0};
#endif // FLANTERM_SUPPORT
__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

uint64_t hhdm_offset = 0;
struct limine_memmap_response *memmap = NULL;
uint64_t kvirt = 0;
uint64_t kphys = 0;
uint64_t kstack_top = 0;
vctx_t *kvm_ctx = NULL;

#if FLANTERM_SUPPORT
struct flanterm_context *ft_ctx = NULL;
#endif // FLANTERM_SUPPORT

void emk_entry(void)
{
    __asm__ volatile("movq %%rsp, %0" : "=r"(kstack_top));
    if (serial_init(COM1) != 0)
    {
        /* Just halt and say nothing */
        hcf();
    }

    /* Init flanterm if we compiled with support */
#if FLANTERM_SUPPORT
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    ft_ctx = flanterm_fb_init(
        NULL,
        NULL,
        framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->pitch,
        framebuffer->red_mask_size, framebuffer->red_mask_shift,
        framebuffer->green_mask_size, framebuffer->green_mask_shift,
        framebuffer->blue_mask_size, framebuffer->blue_mask_shift,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0);
#endif // FLANTERM_SUPPORT

    log_early("Experimental Micro Kernel (EMK) 1.0 Copytright (c) 2025 Piraterna");
    log_early("Compiled at %s %s, emk1.0-%s, flanterm support: %s", __TIME__, __DATE__, BUILD_MODE, FLANTERM_SUPPORT ? "yes" : "no");

    if (!LIMINE_BASE_REVISION_SUPPORTED)
    {
        kpanic(NULL, "Limine base revision is not supported\n");
        hcf();
    }

    gdt_init();
    log_early("Initialized GDT");
    idt_init();
    log_early("Initialized IDT");

    if (!hhdm_request.response)
    {
        kpanic(NULL, "Failed to get HHDM request");
    }

    if (!memmap_request.response)
    {
        kpanic(NULL, "Failed to get memmap request");
    }

    memmap = memmap_request.response;
    hhdm_offset = hhdm_request.response->offset;
    log_early("HHDM Offset: %llx", hhdm_offset);
    pmm_init();

    /* Test allocate a single physical page */
    char *a = palloc(1, true);
    if (!a)
        kpanic(NULL, "Failed to allocate single physical page");

    *a = 32;
    log_early("Allocated 1 physical page @ %llx", (uint64_t)a);
    pfree(a, 1);
    log_early("Initialized physical page manager");

    /* Setup virtual memory */
    if (!kernel_address_request.response)
    {
        kpanic(NULL, "Failed to get kernel address request");
    }

    kvirt = kernel_address_request.response->virtual_base;
    kphys = kernel_address_request.response->physical_base;
    paging_init();
    log_early("Initialized paging");

    /* Kernel Virtual Memory Context, not to be confused with KVM */
    kvm_ctx = vinit(kernel_pagemap, 0x1000);
    if (!kvm_ctx)
    {
        kpanic(NULL, "Failed to create kernel VMM context");
    }

    char *b = valloc(kvm_ctx, 1, VALLOC_RW);
    if (!b)
    {
        kpanic(NULL, "Failed to allocate single virtual page");
    }

    *b = 32;
    log_early("Allocated 1 virtual page @ %llx", (uint64_t)b);
    vfree(kvm_ctx, b);
    log_early("Initialized virtual page manager");

    /* Setup kernel heap */
    heap_init();
    char *c = kmalloc(1);
    if (!c)
    {
        kpanic(NULL, "Failed to allocate single byte on heap");
    }

    *c = 32;
    log_early("Allocated 1 byte @ %llx", (uint64_t)c);
    kfree(c);
    log_early("Initialized kernel heap");

    hlt();
}