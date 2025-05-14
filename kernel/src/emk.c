/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <boot/limine.h>
#include <arch/cpu.h>
#include <arch/io.h>
#include <dev/serial.h>
#include <util/kprintf.h>
#include <util/log.h>
#include <arch/gdt.h>
#include <arch/idt.h>

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);
__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

void emk_entry(void)
{
    if (serial_init(COM1) != 0)
    {
        /* Just halt and say nothing */
        hcf();
    }

    if (!LIMINE_BASE_REVISION_SUPPORTED)
    {
        log_early("ERROR: Limine base revision is not supported\n");
        hcf();
    }

    gdt_init();
    log_early("Initialized GDT");
    idt_init();
    log_early("Initialized IDT");

    hlt();
}