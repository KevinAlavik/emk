/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/ioapic.h>
#include <boot/emk.h>

void ioapic_init()
{
    acpi_madt_ioapic_t *ioapic = madt_ioapic_list[0];

    uint32_t val = ioapic_read(ioapic, IOAPIC_VER);
    uint32_t count = ((val >> 16) & 0xFF);

    if ((ioapic_read(ioapic, 0) >> 24) != ioapic->ioapic_id)
    {
        return;
    }

    for (uint8_t i = 0; i <= count; ++i)
    {
        ioapic_write(ioapic, IOAPIC_REDTBL + 2 * i, 0x00010000 | (32 + i));
        ioapic_write(ioapic, IOAPIC_REDTBL + 2 * i + 1, 0);
    }
}

void ioapic_write(acpi_madt_ioapic_t *ioapic, uint8_t reg, uint32_t val)
{
    *((volatile uint32_t *)(HIGHER_HALF(ioapic->ioapic_addr) + IOAPIC_REGSEL)) =
        reg;
    *((volatile uint32_t *)(HIGHER_HALF(ioapic->ioapic_addr) + IOAPIC_IOWIN)) = val;
}

uint32_t ioapic_read(acpi_madt_ioapic_t *ioapic, uint8_t reg)
{
    *((volatile uint32_t *)(HIGHER_HALF(ioapic->ioapic_addr) + IOAPIC_REGSEL)) =
        reg;
    return *(
        (volatile uint32_t *)(HIGHER_HALF(ioapic->ioapic_addr) + IOAPIC_IOWIN));
}

void ioapic_set_entry(acpi_madt_ioapic_t *ioapic, uint8_t idx, uint64_t data)
{
    ioapic_write(ioapic, (uint8_t)(IOAPIC_REDTBL + idx * 2), (uint32_t)data);
    ioapic_write(ioapic, (uint8_t)(IOAPIC_REDTBL + idx * 2 + 1),
                 (uint32_t)(data >> 32));
}

uint64_t ioapic_gsi_count(acpi_madt_ioapic_t *ioapic)
{
    return (ioapic_read(ioapic, 1) & 0xff0000) >> 16;
}

acpi_madt_ioapic_t *ioapic_get_gsi(uint32_t gsi)
{
    for (uint64_t i = 0; i < madt_iso_len; i++)
    {
        acpi_madt_ioapic_t *ioapic = madt_ioapic_list[i];
        if (ioapic->gsi_base <= gsi &&
            ioapic->gsi_base + ioapic_gsi_count(ioapic) > gsi)
            return ioapic;
    }
    return (acpi_madt_ioapic_t *)0;
}

void ioapic_redirect_gsi(uint32_t lapic_id, uint8_t vec, uint32_t gsi,
                         uint16_t flags, bool mask)
{
    acpi_madt_ioapic_t *ioapic = ioapic_get_gsi(gsi);

    uint64_t redirect = vec;

    if ((flags & (1 << 1)) != 0)
    {
        redirect |= (1 << 13);
    }

    if ((flags & (1 << 3)) != 0)
    {
        redirect |= (1 << 15);
    }

    if (mask)
        redirect |= (1 << 16);
    else
        redirect &= ~(1 << 16);

    redirect |= (uint64_t)lapic_id << 56;

    uint32_t redir_table = (gsi - ioapic->gsi_base) * 2 + 16;
    ioapic_write(ioapic, redir_table, (uint32_t)redirect);
    ioapic_write(ioapic, redir_table + 1, (uint32_t)(redirect >> 32));
}

void ioapic_redirect_irq(uint32_t lapic_id, uint8_t vec, uint8_t irq,
                         bool mask)
{
    uint8_t idx = 0;
    acpi_madt_ioapic_src_ovr_t *iso = (acpi_madt_ioapic_src_ovr_t *)0;

    while (idx < madt_iso_len)
    {
        iso = madt_iso_list[idx];
        if (iso->irq_source == irq)
        {
            ioapic_redirect_gsi(lapic_id, vec, iso->gsi, iso->flags, mask);
            return;
        }
        idx++;
    }

    ioapic_redirect_gsi(lapic_id, vec, irq, 0, mask);
}

uint32_t ioapic_get_redirect_irq(uint8_t irq)
{
    uint8_t idx = 0;
    acpi_madt_ioapic_src_ovr_t *iso;

    while (idx < madt_iso_len)
    {
        iso = madt_iso_list[idx];
        if (iso->irq_source == irq)
        {
            return iso->gsi;
        }
        idx++;
    }

    return irq;
}