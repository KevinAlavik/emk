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
#include <arch/smp.h>
#include <sys/acpi.h>
#include <sys/acpi/madt.h>
#include <sys/apic/lapic.h>
#include <sys/apic/ioapic.h>
#include <dev/pit.h>
#include <sys/syscall.h>

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
__attribute__((used, section(".limine_requests"))) static volatile struct limine_rsdp_request rsdp_request = {
    .revision = 0,
    .id = LIMINE_RSDP_REQUEST};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_mp_request mp_request = {
    .revision = 0,
    .id = LIMINE_MP_REQUEST};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_bootloader_info_request binfo_request = {
    .revision = 0,
    .id = LIMINE_BOOTLOADER_INFO_REQUEST};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_firmware_type_request firmware_request = {
    .revision = 0,
    .id = LIMINE_FIRMWARE_TYPE_REQUEST};
__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

uint64_t hhdm_offset = 0;
struct limine_memmap_response *memmap = NULL;
uint64_t kvirt = 0;
uint64_t kphys = 0;
uint64_t kstack_top = 0;
vctx_t *kvm_ctx = NULL;
struct limine_rsdp_response *rsdp_response = NULL;
struct limine_mp_response *mp_response = NULL;

#if FLANTERM_SUPPORT
struct flanterm_context *ft_ctx = NULL;
#endif // FLANTERM_SUPPORT

void tick(struct register_ctx *)
{
    log_early("tick on CPU %d", get_cpu_local()->cpu_index);
}

void user_func()
{
    syscall(69, 69, 420, 420);
}

void emk_entry(void)
{
    __asm__ volatile("movq %%rsp, %0" : "=r"(kstack_top));

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

    if (serial_init(COM1) != 0)
    {
        /* Just do nothing */
    }

    if (!binfo_request.response)
    {
        kpanic(NULL, "Failed to get bootloader info");
    }

    if (!firmware_request.response)
    {
        kpanic(NULL, "Failed to get firmware type");
    }

    log_early("Experimental Micro Kernel (EMK) 1.0 Copyright (c) 2025 Piraterna");
    log_early("Compiled at %s %s, emk1.0-%s, flanterm support: %s, bootloader: %s v%s, firmware: %s", __TIME__, __DATE__, BUILD_MODE, FLANTERM_SUPPORT ? "yes" : "no", binfo_request.response->name, binfo_request.response->version, (firmware_request.response->firmware_type == LIMINE_FIRMWARE_TYPE_UEFI64 || firmware_request.response->firmware_type == LIMINE_FIRMWARE_TYPE_UEFI32) ? "UEFI" : "BIOS");
    log_early("%s", LOG_SEPARATOR);

    if (!LIMINE_BASE_REVISION_SUPPORTED)
    {
        kpanic(NULL, "Limine base revision is not supported");
        hcf();
    }

    gdt_init();
    idt_init();

    /* Setup physical memory*/
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
    pmm_init();

    /* Test allocate a single physical page */
    char *a = palloc(1, true);
    if (!a)
        kpanic(NULL, "Failed to allocate single physical page");

    *a = 32;
    pfree(a, 1);

    /* Setup virtual memory */
    if (!kernel_address_request.response)
    {
        kpanic(NULL, "Failed to get kernel address request");
    }

    kvirt = kernel_address_request.response->virtual_base;
    kphys = kernel_address_request.response->physical_base;
    paging_init();

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
    vfree(kvm_ctx, b);

    /* Setup kernel heap */
    heap_init();
    char *c = kmalloc(1);
    if (!c)
    {
        kpanic(NULL, "Failed to allocate single byte on heap");
    }

    *c = 32;
    kfree(c);

    /* Setup ACPI */
    rsdp_response = rsdp_request.response;
    if (!rsdp_response)
    {
        kpanic(NULL, "Failed to get RSDP request");
    }
    acpi_init();
    madt_init(); // Also init MADT, to prepare for APIC

    /* Disable legacy PIC to prepare for APIC */
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    /* Setup SMP */
    if (!mp_request.response)
    {
        kpanic(NULL, "Failed to get MP request");
    }

    mp_response = mp_request.response;
    lapic_init();
    smp_init();

    /* Setup IOAPIC */
    ioapic_init();

    /* Setup timer */
    pit_init(NULL);

    /* Call our user space function */
    user_func(); // No good way to run in usermode, yet

    /* Finished */
    log_early("%s", LOG_SEPARATOR);
    log_early("Finished initializing EMK v1.0, took ? seconds"); /* Still not usermode, so keep using log_early */

    __asm__ volatile("sti");
    hlt();
}