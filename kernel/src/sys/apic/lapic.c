/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/apic/lapic.h>
#include <arch/cpu.h>
#include <sys/acpi/madt.h>
#include <util/log.h>
#include <mm/vmm.h>
#include <stdatomic.h>

atomic_uintptr_t lapic_msr = 0;
volatile uint64_t *lapic_base = 0;

void lapic_write(uint32_t offset, uint32_t value)
{
    if (!lapic_base)
    {
        log_early("warning: LAPIC not initialized!");
        return;
    }

    volatile uint32_t *reg = (volatile uint32_t *)((uint8_t *)lapic_base + offset);
    atomic_store((_Atomic uint32_t *)reg, value);
}

uint32_t lapic_read(uint32_t offset)
{
    if (!lapic_base)
    {
        log_early("warning: LAPIC not initialized!");
        return 0;
    }

    volatile uint32_t *reg = (volatile uint32_t *)((uint8_t *)lapic_base + offset);
    return atomic_load((_Atomic uint32_t *)reg);
}

void lapic_init()
{
    uint64_t msr = rdmsr(LAPIC_BASE);
    atomic_store(&lapic_msr, msr);
    lapic_base = (volatile uint64_t *)(msr & ~(0xffff));
    atomic_store(&lapic_addr, (uint64_t)lapic_base);
    log_early("New LAPIC base: 0x%lx", lapic_base);
}