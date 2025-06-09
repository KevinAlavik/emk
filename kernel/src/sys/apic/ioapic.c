/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/apic/ioapic.h>
#include <sys/acpi/madt.h>
#include <sys/kpanic.h>
#include <stdatomic.h>
#include <util/log.h>
#include <arch/paging.h>
#include <arch/idt.h>
#include <arch/smp.h>
#include <arch/io.h>
#include <sys/apic/lapic.h>

static atomic_uintptr_t ioapic_base = 0;

// Helper function to translate IRQ to GSI based on MADT ISO entries
static uint32_t irq_to_gsi(uint32_t irq)
{
    for (uint32_t i = 0; i < madt_iso_len; i++)
    {
        struct acpi_madt_ioapic_src_ovr *iso = madt_iso_list[i];
        if (iso->irq_source == irq)
        {
            return iso->gsi;
        }
    }
    // No override; assume IRQ == GSI
    return irq;
}

void ioapic_write(uint8_t index, uint32_t value)
{
    volatile uint32_t *ioapic = (volatile uint32_t *)atomic_load(&ioapic_base);
    if (!ioapic)
    {
        log_early("error: IOAPIC not initialized");
        kpanic(NULL, "IOAPIC write before init");
    }
    ioapic[IOAPIC_OFF_IOREGSEL / 4] = index;
    ioapic[IOAPIC_OFF_IOWIN / 4] = value;
}

uint32_t ioapic_read(uint8_t index)
{
    volatile uint32_t *ioapic = (volatile uint32_t *)atomic_load(&ioapic_base);
    if (!ioapic)
    {
        log_early("error: IOAPIC not initialized");
        return 0;
    }
    ioapic[IOAPIC_OFF_IOREGSEL / 4] = index;
    return ioapic[IOAPIC_OFF_IOWIN / 4];
}

void ioapic_map(int irq, int vec, uint8_t dest_mode, uint8_t lapic_id)
{
    uint32_t gsi = irq_to_gsi(irq);
    uint32_t max_irqs = ((ioapic_read(IOAPIC_IDX_IOAPICVER) >> 16) & 0xFF) + 1;
    if (gsi >= max_irqs)
    {
        log_early("error: Invalid GSI %u for IRQ %d (max %u)", gsi, irq, max_irqs);
        kpanic(NULL, "Invalid GSI for IOAPIC");
    }
    if (vec < 32 || vec > 255 || vec == LAPIC_SPURIOUS_VECTOR)
    {
        log_early("error: Invalid vector 0x%x for IRQ %d (GSI %u)", vec, irq, gsi);
        kpanic(NULL, "Invalid vector");
    }

    uint32_t redtble_lo = (1 << 16) |         // Masked by default
                          (dest_mode << 11) | // 0: Physical, 1: Logical
                          (0 << 8) |          // Fixed delivery
                          vec;
    uint32_t redtble_hi = (lapic_id << 24);
    ioapic_write(0x10 + 2 * gsi, redtble_lo);
    ioapic_write(0x10 + 2 * gsi + 1, redtble_hi);
}

void ioapic_unmask(int irq)
{
    uint32_t gsi = irq_to_gsi(irq);
    uint32_t max_irqs = ((ioapic_read(IOAPIC_IDX_IOAPICVER) >> 16) & 0xFF) + 1;
    if (gsi >= max_irqs)
    {
        log_early("error: Invalid GSI %u for IRQ %d (max %u)", gsi, irq, max_irqs);
        return;
    }
    uint32_t redtble_lo = ioapic_read(0x10 + 2 * gsi);
    redtble_lo &= ~(1 << 16);
    ioapic_write(0x10 + 2 * gsi, redtble_lo);
}

void ioapic_init(void)
{
    if (madt_ioapic_len < 1)
    {
        log_early("error: No IOAPIC entries in MADT");
        kpanic(NULL, "No IOAPIC available");
    }

    uint64_t phys_addr = madt_ioapic_list[0]->ioapic_addr;
    if (phys_addr & 0xFFF)
    {
        log_early("error: IOAPIC base 0x%lx not page-aligned", phys_addr);
        kpanic(NULL, "Invalid IOAPIC alignment");
    }
    uint64_t virt_addr = (uint64_t)HIGHER_HALF(phys_addr);
    int ret = vmap(pmget(), virt_addr, phys_addr, VMM_PRESENT | VMM_WRITE | VMM_NX);
    if (ret != 0)
    {
        log_early("error: Failed to map IOAPIC 0x%lx to 0x%lx (%d)", phys_addr, virt_addr, ret);
        kpanic(NULL, "IOAPIC mapping failed");
    }
    atomic_store(&ioapic_base, virt_addr);

    uint32_t ioapic_ver = ioapic_read(IOAPIC_IDX_IOAPICVER);
    uint32_t max_irqs = ((ioapic_ver >> 16) & 0xFF) + 1;

    // Initialize all GSIs, respecting ISO overrides
    for (uint32_t gsi = 0; gsi < max_irqs; gsi++)
    {
        uint32_t vec = 32 + gsi;
        if (vec == LAPIC_SPURIOUS_VECTOR)
        {
            vec++;
        }
        // Check if this GSI is overridden
        int is_overridden = 0;
        uint32_t irq = gsi; // Default: GSI == IRQ
        for (uint32_t i = 0; i < madt_iso_len; i++)
        {
            if (madt_iso_list[i]->gsi == gsi)
            {
                is_overridden = 1;
                irq = madt_iso_list[i]->irq_source;
                break;
            }
        }
        uint32_t redtble_lo = (1 << 16) | // Masked by default
                              (0 << 11) | // Physical mode
                              (0 << 8) |  // Fixed delivery
                              vec;
        uint32_t redtble_hi = (get_cpu_local()->lapic_id << 24);
        ioapic_write(0x10 + 2 * gsi, redtble_lo);
        ioapic_write(0x10 + 2 * gsi + 1, redtble_hi);
        if (is_overridden)
        {
            log_early("Initialized IRQ %u (GSI %u) to vector 0x%x", irq, gsi, vec);
        }
        else
        {
            log_early("Initialized GSI %u to vector 0x%x", gsi, vec);
        }
    }
}
