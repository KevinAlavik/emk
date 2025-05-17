/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/ioapic.h>
#include <boot/emk.h>
#include <util/log.h>

void ioapic_init()
{
    if (!madt_ioapic_list[0])
    {
        return;
    }

    acpi_madt_ioapic_t *ioapic = madt_ioapic_list[0];

    uint32_t val = ioapic_read(ioapic, IOAPIC_VER);
    uint32_t count = ((val >> 16) & 0xFF) + 1;

    if ((ioapic_read(ioapic, IOAPIC_ID) >> 24) != ioapic->ioapic_id)
    {
        return;
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        ioapic_write(ioapic, IOAPIC_REDTBL + 2 * i, (32 + i));
        ioapic_write(ioapic, IOAPIC_REDTBL + 2 * i + 1, 0);
    }
}

void ioapic_write(acpi_madt_ioapic_t *ioapic, uint8_t reg, uint32_t val)
{
    if (!ioapic)
    {
        return;
    }

    volatile uint32_t *regsel = (volatile uint32_t *)(HIGHER_HALF(ioapic->ioapic_addr) + IOAPIC_REGSEL);
    volatile uint32_t *iowin = (volatile uint32_t *)(HIGHER_HALF(ioapic->ioapic_addr) + IOAPIC_IOWIN);

    *regsel = reg;
    *iowin = val;
}

uint32_t ioapic_read(acpi_madt_ioapic_t *ioapic, uint8_t reg)
{
    if (!ioapic)
    {
        return 0;
    }

    volatile uint32_t *regsel = (volatile uint32_t *)(HIGHER_HALF(ioapic->ioapic_addr) + IOAPIC_REGSEL);
    volatile uint32_t *iowin = (volatile uint32_t *)(HIGHER_HALF(ioapic->ioapic_addr) + IOAPIC_IOWIN);

    *regsel = reg;
    return *iowin;
}

void ioapic_set_entry(acpi_madt_ioapic_t *ioapic, uint8_t idx, uint64_t data)
{
    if (!ioapic)
    {
        return;
    }

    ioapic_write(ioapic, IOAPIC_REDTBL + idx * 2, (uint32_t)(data & 0xFFFFFFFF));
    ioapic_write(ioapic, IOAPIC_REDTBL + idx * 2 + 1, (uint32_t)(data >> 32));
}

uint64_t ioapic_gsi_count(acpi_madt_ioapic_t *ioapic)
{
    if (!ioapic)
    {
        return 0;
    }

    return ((ioapic_read(ioapic, IOAPIC_VER) >> 16) & 0xFF) + 1;
}

acpi_madt_ioapic_t *ioapic_get_gsi(uint32_t gsi)
{
    for (uint32_t i = 0; i < madt_iso_len; i++)
    {
        if (!madt_ioapic_list[i])
        {
            continue;
        }

        acpi_madt_ioapic_t *ioapic = madt_ioapic_list[i];
        uint64_t gsi_count = ioapic_gsi_count(ioapic);

        if (ioapic->gsi_base <= gsi && ioapic->gsi_base + gsi_count > gsi)
        {
            return ioapic;
        }
    }

    return NULL;
}

void ioapic_redirect_gsi(uint32_t lapic_id, uint8_t vec, uint32_t gsi, uint16_t flags, bool mask)
{
    acpi_madt_ioapic_t *ioapic = ioapic_get_gsi(gsi);
    if (!ioapic)
    {
        return;
    }

    uint64_t redirect = vec;

    if (flags & (1 << 1))
    {
        redirect |= (1ULL << 13);
    }

    if (flags & (1 << 3))
    {
        redirect |= (1ULL << 15);
    }

    if (mask)
    {
        redirect |= (1ULL << 16);
    }
    else
    {
        redirect &= ~(1ULL << 16);
    }

    redirect |= (uint64_t)lapic_id << 56;

    uint32_t redir_table = (gsi - ioapic->gsi_base) * 2 + IOAPIC_REDTBL;
    ioapic_write(ioapic, redir_table, (uint32_t)(redirect & 0xFFFFFFFF));
    ioapic_write(ioapic, redir_table + 1, (uint32_t)(redirect >> 32));
}

void ioapic_redirect_irq(uint32_t lapic_id, uint8_t vec, uint8_t irq, bool mask)
{
    log_early("IOAPIC redirect: IRQ=%u -> Vector=0x%X (mask=%s)", irq, vec, mask ? "true" : "false");

    for (uint32_t idx = 0; idx < madt_iso_len; idx++)
    {
        if (!madt_iso_list[idx])
        {
            continue;
        }

        acpi_madt_ioapic_src_ovr_t *iso = madt_iso_list[idx];

        log_early("  ISO override found: irq_source=%u, gsi=%u, flags=0x%X",
                  iso->irq_source, iso->gsi, iso->flags);

        if (iso->irq_source == irq)
        {
            log_early("  Match found. Redirecting IRQ %u (GSI %u) to vector 0x%X",
                      irq, iso->gsi, vec);
            ioapic_redirect_gsi(lapic_id, vec, iso->gsi, iso->flags, mask);
            return;
        }
    }

    log_early("  No ISO match. Redirecting IRQ %u directly to vector 0x%X", irq, vec);
    ioapic_redirect_gsi(lapic_id, vec, irq, 0, mask);
}

uint32_t ioapic_get_redirect_irq(uint8_t irq)
{
    for (uint32_t idx = 0; idx < madt_iso_len; idx++)
    {
        if (!madt_iso_list[idx])
        {
            continue;
        }

        acpi_madt_ioapic_src_ovr_t *iso = madt_iso_list[idx];
        if (iso->irq_source == irq)
        {
            return iso->gsi;
        }
    }

    return irq;
}