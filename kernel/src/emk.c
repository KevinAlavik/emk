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
__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

uint64_t hhdm_offset = 0;
struct limine_memmap_response *memmap = NULL;
uint64_t kvirt = 0;
uint64_t kphys = 0;
uint64_t kstack_top = 0;

void emk_entry(void)
{
    __asm__ volatile("movq %%rsp, %0" : "=r"(kstack_top));
    if (serial_init(COM1) != 0)
    {
        /* Just halt and say nothing */
        hcf();
    }
    log_early("Experimental Micro Kernel (EMK) 1.0 Copytright (c) 2025 Piraterna");
    log_early("Compiled at %s %s", __TIME__, __DATE__);

    if (!LIMINE_BASE_REVISION_SUPPORTED)
    {
        log_early("ERROR: Limine base revision is not supported\n");
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
    log_early("Initialized PMM");

    /* Test allocate a single physical page */
    char *a = palloc(1, true);
    if (!a)
        kpanic(NULL, "Failed to allocate single physical page");

    *a = 32;
    log_early("Allocated 1 physical page: %llx", (uint64_t)a);
    pfree(a, 1);

    paging_init();

    hlt();
}