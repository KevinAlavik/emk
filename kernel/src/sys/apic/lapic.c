/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/apic/lapic.h>
#include <arch/cpu.h>
#include <arch/paging.h>
#include <util/log.h>
#include <stdatomic.h>
#include <sys/kpanic.h>

#define LAPIC_REG_ALIGN 16
#define LAPIC_REG_SIZE 4

atomic_uintptr_t lapic_msr = 0;
atomic_uintptr_t lapic_base = 0;

#define LAPIC_BASE ((volatile uint32_t *)atomic_load(&lapic_base))

void lapic_write(uint32_t offset, uint32_t value)
{
    volatile uint32_t *base = LAPIC_BASE;
    if (!base)
    {
        log_early("error: LAPIC not initialized!");
        kpanic(NULL, "LAPIC write attempted before initialization");
    }
    if (offset % LAPIC_REG_ALIGN != 0)
    {
        log_early("error: Misaligned LAPIC offset 0x%x", offset);
        kpanic(NULL, "Invalid LAPIC register offset");
    }
    volatile uint32_t *reg = base + (offset / LAPIC_REG_SIZE);
    *reg = value;
}

uint32_t lapic_read(uint32_t offset)
{
    volatile uint32_t *base = LAPIC_BASE;
    if (!base)
    {
        log_early("error: LAPIC not initialized!");
        return 0;
    }
    if (offset % LAPIC_REG_ALIGN != 0)
    {
        log_early("error: Misaligned LAPIC offset 0x%x", offset);
        kpanic(NULL, "Invalid LAPIC register offset");
    }
    volatile uint32_t *reg = base + (offset / LAPIC_REG_SIZE);
    return *reg;
}

void lapic_init(void)
{
    uint64_t msr = rdmsr(LAPIC_BASE_MSR);
    msr |= (1 << 11); // Set global LAPIC enable bit
    wrmsr(LAPIC_BASE_MSR, msr);
    atomic_store(&lapic_msr, msr);

    uint64_t phys_addr = msr & ~0xFFFULL;
    uint64_t virt_addr = (uint64_t)HIGHER_HALF(phys_addr);

    int ret = vmap(pmget(), virt_addr, phys_addr, VMM_PRESENT | VMM_WRITE | VMM_NX);
    if (ret != 0)
    {
        log_early("error: Failed to map LAPIC base 0x%lx to 0x%lx", phys_addr, virt_addr);
        kpanic(NULL, "LAPIC mapping failed");
    }

    atomic_store(&lapic_base, virt_addr);
    atomic_store(&lapic_addr, virt_addr);
}

void lapic_eoi(void)
{
    lapic_write(LAPIC_EOI, 0);
}

void lapic_enable(void)
{
    volatile uint32_t *base = LAPIC_BASE;
    if (!base)
    {
        log_early("warning: lapic_enable called before lapic_init");
        return;
    }

    uint32_t svr = lapic_read(LAPIC_SVR);
    svr |= (1 << 8);            // Enable APIC
    svr &= ~(1 << 9);           // Disable focus processor checking
    svr = (svr & ~0xFF) | 0xFF; // Set spurious interrupt vector to 0xFF

    lapic_write(LAPIC_SVR, svr);
    lapic_write(LAPIC_TPR, 0);
    uint32_t id = lapic_read(LAPIC_ID) >> 24;
    log_early("LAPIC enabled and initialized for CPU %u", id);
}