/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/apic/ioapic.h>
#include <sys/acpi/madt.h>
#include <sys/kpanic.h>
#include <stdatomic.h>
#include <util/log.h>
#include <arch/paging.h>
#include <boot/emk.h>
#include <arch/idt.h>
#include <arch/smp.h>

atomic_uintptr_t ioapic_base = 0;

void ioapic_write(uint8_t index, uint32_t value)
{
    volatile uint32_t *ioapic = (volatile uint32_t *)atomic_load(&ioapic_base);
    ioapic[IOAPIC_OFF_IOREGSEL / 4] = index; // Write register index to IOREGSEL
    ioapic[IOAPIC_OFF_IOWIN / 4] = value;    // Write value to IOWIN
}

uint32_t ioapic_read(uint8_t index)
{
    volatile uint32_t *ioapic = (volatile uint32_t *)atomic_load(&ioapic_base);
    ioapic[IOAPIC_OFF_IOREGSEL / 4] = index; // Write register index to IOREGSEL
    return ioapic[IOAPIC_OFF_IOWIN / 4];     // Read value from IOWIN
}

void ioapic_map(int irq, int vec, idt_intr_handler handler)
{
    uint32_t redtble_lo = (0 << 16) | /* Unmask the entry */
                          (0 << 11) | /* Dest mode */
                          (0 << 8) |  /* Delivery mode */
                          vec;        /* Interrupt vector*/
    ioapic_write(2 * irq, redtble_lo);
    uint32_t redtble_hi = (get_cpu_local()->lapic_id << 24);
    ioapic_write(2 * irq + 1, redtble_hi);
    idt_register_handler(vec, handler);
}

void ioapic_init()
{
    if (madt_ioapic_len < 1)
    {
        kpanic(NULL, "No I/O APIC's available");
    }

    /* Set base of I/O APIC */
    uint64_t base = madt_ioapic_list[0]->ioapic_addr;
    log_early("I/O APIC phys addr: 0x%lx", base);
    uint64_t virt_addr = (uint64_t)HIGHER_HALF(base);

    int ret = vmap(pmget(), virt_addr, base, VMM_PRESENT | VMM_WRITE | VMM_NX);
    if (ret != 0)
    {
        log_early("error: Failed to map I/O APIC base 0x%lx to 0x%lx", base, virt_addr);
        kpanic(NULL, "I/O APIC mapping failed");
    }
    atomic_store(&ioapic_base, virt_addr);

    // Read IOAPICVER register to get the maximum redirection entry
    uint32_t ioapic_ver = ioapic_read(IOAPIC_IDX_IOAPICVER);
    uint32_t max_irqs = ((ioapic_ver >> 16) & 0xFF) + 1;
    log_early("I/O APIC supports %u IRQs", max_irqs);
}