/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/apic/lapic.h>
#include <arch/cpu.h>
#include <sys/acpi/madt.h>
#include <util/log.h>
#include <mm/vmm.h>
#include <boot/emk.h>

uint64_t lapic_msr = 0;
volatile uint64_t *lapic_base = 0;

void lapic_write(uint32_t offset, uint32_t value)
{
    if (!lapic_base)
    {
        log_early("warning: LAPIC not initialized!");
        return;
    }
    volatile uint32_t *reg = (volatile uint32_t *)((uint8_t *)lapic_base + offset);
    *reg = value;
}

uint32_t lapic_read(uint32_t offset)
{
    if (!lapic_base)
    {
        log_early("warning: LAPIC not initialized!");
        return 0;
    }
    volatile uint32_t *reg = (volatile uint32_t *)((uint8_t *)lapic_base + offset);
    uint32_t value = *reg;
    return value;
}

void lapic_init()
{
    lapic_msr = rdmsr(LAPIC_BASE);
    lapic_base = (volatile uint64_t *)(lapic_msr & ~(0xffff));
    /* Change the lapic address that we got from MADT earlier */
    lapic_addr = (uint64_t)lapic_base;
    log_early("New LAPIC base: 0x%lx", lapic_base);
}