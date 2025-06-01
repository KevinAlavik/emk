/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/apic/lapic.h>
#include <arch/cpu.h>
#include <arch/paging.h>
#include <util/log.h>
#include <stdatomic.h>
#include <sys/kpanic.h>

#define LAPIC_REG_ALIGN 16
#define LAPIC_REG_SIZE 4

static atomic_uintptr_t lapic_msr = 0;
static atomic_uintptr_t lapic_base = 0;

#define LAPIC_BASE ((volatile uint32_t *)atomic_load(&lapic_base))

void lapic_write(uint32_t offset, uint32_t value)
{
    volatile uint32_t *base = LAPIC_BASE;
    if (!base)
    {
        log_early("error: LAPIC not initialized");
        kpanic(NULL, "LAPIC write before init");
    }
    if (offset % LAPIC_REG_ALIGN != 0)
    {
        log_early("error: Misaligned LAPIC offset 0x%x", offset);
        kpanic(NULL, "Invalid LAPIC offset");
    }
    base[offset / LAPIC_REG_SIZE] = value;
}

uint32_t lapic_read(uint32_t offset)
{
    volatile uint32_t *base = LAPIC_BASE;
    if (!base)
    {
        log_early("error: LAPIC not initialized");
        return 0;
    }
    if (offset % LAPIC_REG_ALIGN != 0)
    {
        log_early("error: Misaligned LAPIC offset 0x%x", offset);
        kpanic(NULL, "Invalid LAPIC offset");
    }
    return base[offset / LAPIC_REG_SIZE];
}

uint32_t lapic_get_id(void)
{
    volatile uint32_t *base = LAPIC_BASE;
    if (!base)
    {
        log_early("error: LAPIC not initialized for ID read");
        kpanic(NULL, "LAPIC get_id before init");
    }
    uint32_t id = lapic_read(LAPIC_ID) >> 24;
    if (id > 255)
    {
        log_early("error: Invalid LAPIC ID %u", id);
        kpanic(NULL, "Invalid LAPIC ID");
    }
    return id;
}

void lapic_init(void)
{
    uint64_t msr = rdmsr(LAPIC_BASE_MSR);
    if (!(msr & (1ULL << 11)))
    {
        log_early("LAPIC disabled in MSR, enabling");
    }
    msr |= (1ULL << 11);
    wrmsr(LAPIC_BASE_MSR, msr);
    atomic_store(&lapic_msr, msr);

    uint64_t phys_addr = msr & ~0xFFFULL;
    if (phys_addr & 0xFFF)
    {
        log_early("error: LAPIC base 0x%lx not page-aligned", phys_addr);
        kpanic(NULL, "Invalid LAPIC alignment");
    }
    uint64_t virt_addr = (uint64_t)HIGHER_HALF(phys_addr);
    int ret = vmap(pmget(), virt_addr, phys_addr, VMM_PRESENT | VMM_WRITE | VMM_NX);
    if (ret != 0)
    {
        log_early("error: Failed to map LAPIC 0x%lx to 0x%lx (%d)", phys_addr, virt_addr, ret);
        kpanic(NULL, "LAPIC mapping failed");
    }
    atomic_store(&lapic_base, virt_addr);

    uint32_t id = lapic_get_id(); // Use the new function
    lapic_write(LAPIC_EOI, 0);
}

void lapic_enable(void)
{
    volatile uint32_t *base = LAPIC_BASE;
    if (!base)
    {
        log_early("warning: lapic_enable before init");
        return;
    }

    uint32_t svr = lapic_read(LAPIC_SVR);
    svr |= (1 << 8);
    svr &= ~(1 << 9);
    svr = (svr & ~0xFF) | LAPIC_SPURIOUS_VECTOR;
    lapic_write(LAPIC_SVR, svr);

    /* Mask Timer */
    uint32_t timer = lapic_read(LAPIC_LVT_TIMER);
    timer |= (1 << 16);
    lapic_write(LAPIC_LVT_TIMER, timer);

    /* Mask LVT0 */
    uint32_t lvt0 = lapic_read(LAPIC_LVT_LINT0);
    lvt0 |= (1 << 16);
    lapic_write(LAPIC_LVT_LINT0, lvt0);

    /* Mask LVT1 */
    uint32_t lvt1 = lapic_read(LAPIC_LVT_LINT1);
    lvt1 |= (1 << 16);
    lapic_write(LAPIC_LVT_LINT1, lvt1);

    lapic_write(LAPIC_TPR, 0);
}

void lapic_eoi(void)
{
    lapic_write(LAPIC_EOI, 0);
}

void lapic_send_ipi(uint32_t apic_id, uint32_t vector, uint32_t delivery_mode, uint32_t dest_mode, uint32_t shorthand)
{
    volatile uint32_t *base = LAPIC_BASE;
    if (!base)
    {
        log_early("error: LAPIC not initialized for IPI");
        kpanic(NULL, "LAPIC IPI before init");
    }

    if (vector > 255 && delivery_mode != ICR_INIT && delivery_mode != ICR_STARTUP)
    {
        log_early("error: Invalid vector 0x%x for IPI", vector);
        kpanic(NULL, "Invalid IPI vector");
    }
    if (apic_id > 255 && shorthand == ICR_NO_SHORTHAND)
    {
        log_early("error: Invalid APIC ID %u for IPI", apic_id);
        kpanic(NULL, "Invalid APIC ID");
    }

    while (lapic_read(LAPIC_ICRLO) & ICR_SEND_PENDING)
    {
        __asm__ volatile("pause");
    }

    if (shorthand == ICR_NO_SHORTHAND)
    {
        lapic_write(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
    }
    else
    {
        lapic_write(LAPIC_ICRHI, 0);
    }

    uint32_t icr_lo = vector | delivery_mode | dest_mode | ICR_ASSERT | ICR_EDGE | shorthand;
    lapic_write(LAPIC_ICRLO, icr_lo);

    if (delivery_mode != ICR_INIT && delivery_mode != ICR_STARTUP)
    {
        while (lapic_read(LAPIC_ICRLO) & ICR_SEND_PENDING)
        {
            __asm__ volatile("pause");
        }
    }
}