/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/acpi/madt.h>
#include <sys/acpi.h>
#include <sys/kpanic.h>
#include <util/log.h>

acpi_madt_ioapic_t *madt_ioapic_list[256] = {0};
acpi_madt_ioapic_src_ovr_t *madt_iso_list[256] = {0};

uint32_t madt_ioapic_len = 0;
uint32_t madt_iso_len = 0;

uint64_t lapic_addr = 0;

void madt_init()
{
    acpi_madt_t *madt = (acpi_madt_t *)acpi_find_table("APIC");
    if (!madt)
    {
        kpanic(NULL, "Failed to find MADT table");
    }

    if (madt->sdt.length < sizeof(acpi_madt_t))
    {
        kpanic(NULL, "MADT table is too small");
    }

    uint64_t offset = 0;
    while (offset < madt->sdt.length - sizeof(acpi_madt_t))
    {
        acpi_madt_entry_t *entry = (acpi_madt_entry_t *)(madt->table + offset);

        if (entry->length == 0 || offset + entry->length > madt->sdt.length - sizeof(acpi_madt_t))
        {
            kpanic(NULL, "Invalid MADT entry length or overflow");
        }

        if (entry->type == MADT_ENTRY_IOAPIC)
        {
            if (madt_ioapic_len < 256)
                madt_ioapic_list[madt_ioapic_len++] = (acpi_madt_ioapic_t *)entry;
        }
        else if (entry->type == MADT_ENTRY_IOAPIC_SRC_OVR)
        {
            if (madt_iso_len < 256)
                madt_iso_list[madt_iso_len++] = (acpi_madt_ioapic_src_ovr_t *)entry;
        }
        else if (entry->type == MADT_ENTRY_LAPIC_ADDR_OVR)
        {
            lapic_addr = ((acpi_madt_lapic_addr_ovr_t *)entry)->lapic_addr;
        }

        offset += entry->length;
    }
    log_early("MADT parsed: %u IOAPIC(s), %u ISO(s), LAPIC addr: 0x%lx", madt_ioapic_len, madt_iso_len, lapic_addr);
}