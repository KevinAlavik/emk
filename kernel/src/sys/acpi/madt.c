/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/acpi/madt.h>
#include <sys/acpi.h>
#include <sys/kpanic.h>
#include <util/log.h>

acpi_madt_ioapic_t *madt_ioapic_list[256] = {0};
acpi_madt_ioapic_src_ovr_t *madt_iso_list[256] = {0};
acpi_madt_lapic_t *madt_lapic_list[256] = {0};
acpi_madt_lapic_nmi_t *madt_lapic_nmi_list[256] = {0};
acpi_madt_ioapic_nmi_src_t *madt_ioapic_nmi_list[256] = {0};
acpi_madt_lx2apic_t *madt_lx2apic_list[256] = {0};

uint32_t madt_ioapic_len = 0;
uint32_t madt_iso_len = 0;
uint32_t madt_lapic_len = 0;
uint32_t madt_lapic_nmi_len = 0;
uint32_t madt_ioapic_nmi_len = 0;
uint32_t madt_lx2apic_len = 0;

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

    lapic_addr = madt->lapic_base;

    uint64_t offset = 0;
    while (offset < madt->sdt.length - sizeof(acpi_madt_t))
    {
        acpi_madt_entry_t *entry = (acpi_madt_entry_t *)(madt->table + offset);
        if (entry->length == 0 || offset + entry->length > madt->sdt.length - sizeof(acpi_madt_t))
        {
            kpanic(NULL, "Invalid MADT entry length or overflow");
        }

        switch (entry->type)
        {
        case MADT_ENTRY_LAPIC: // Type 0: Local APIC
        {
            acpi_madt_lapic_t *lapic = (acpi_madt_lapic_t *)entry;
            if (entry->length < sizeof(acpi_madt_lapic_t))
            {
                kpanic(NULL, "MADT Local APIC entry too small");
            }
            if (madt_lapic_len < 256)
            {
                madt_lapic_list[madt_lapic_len++] = lapic;
            }
            log_early("MADT: Found Local APIC, proc_id=%u, apic_id=%u, flags=0x%x",
                      lapic->acpi_proc_id, lapic->apic_id, lapic->flags);
            break;
        }
        case MADT_ENTRY_IOAPIC: // Type 1: I/O APIC
        {
            acpi_madt_ioapic_t *ioapic = (acpi_madt_ioapic_t *)entry;
            if (entry->length < sizeof(acpi_madt_ioapic_t))
            {
                kpanic(NULL, "MADT I/O APIC entry too small");
            }
            if (madt_ioapic_len < 256)
            {
                madt_ioapic_list[madt_ioapic_len++] = ioapic;
            }
            log_early("MADT: Found I/O APIC, ioapic_id=%u, addr=0x%x, gsi_base=%u",
                      ioapic->ioapic_id, ioapic->ioapic_addr, ioapic->gsi_base);
            break;
        }
        case MADT_ENTRY_IOAPIC_SRC_OVR: // Type 2: Interrupt Source Override
        {
            acpi_madt_ioapic_src_ovr_t *iso = (acpi_madt_ioapic_src_ovr_t *)entry;
            if (entry->length < sizeof(acpi_madt_ioapic_src_ovr_t))
            {
                kpanic(NULL, "MADT I/O APIC ISO entry too small");
            }
            if (madt_iso_len < 256)
            {
                madt_iso_list[madt_iso_len++] = iso;
            }
            log_early("MADT: Found I/O APIC ISO, bus=%u, irq=%u, gsi=%u, flags=0x%x",
                      iso->bus_source, iso->irq_source, iso->gsi, iso->flags);
            break;
        }
        case MADT_ENTRY_IOAPIC_NMI_SRC: // Type 3: I/O APIC NMI Source
        {
            acpi_madt_ioapic_nmi_src_t *nmi_src = (acpi_madt_ioapic_nmi_src_t *)entry;
            if (entry->length < sizeof(acpi_madt_ioapic_nmi_src_t))
            {
                kpanic(NULL, "MADT I/O APIC NMI Source entry too small");
            }
            if (madt_ioapic_nmi_len < 256)
            {
                madt_ioapic_nmi_list[madt_ioapic_nmi_len++] = nmi_src;
            }
            log_early("MADT: Found I/O APIC NMI Source, nmi_source=%u, gsi=%u, flags=0x%x",
                      nmi_src->nmi_source, nmi_src->gsi, nmi_src->flags);
            break;
        }
        case MADT_ENTRY_LAPIC_NMI: // Type 4: Local APIC NMI
        {
            acpi_madt_lapic_nmi_t *lapic_nmi = (acpi_madt_lapic_nmi_t *)entry;
            if (entry->length < sizeof(acpi_madt_lapic_nmi_t))
            {
                kpanic(NULL, "MADT Local APIC NMI entry too small");
            }
            if (madt_lapic_nmi_len < 256)
            {
                madt_lapic_nmi_list[madt_lapic_nmi_len++] = lapic_nmi;
            }
            log_early("MADT: Found Local APIC NMI, proc_id=%u, lint=%u, flags=0x%x",
                      lapic_nmi->acpi_proc_id, lapic_nmi->lint, lapic_nmi->flags);
            break;
        }
        case MADT_ENTRY_LAPIC_ADDR_OVR: // Type 5: LAPIC Address Override
        {
            acpi_madt_lapic_addr_ovr_t *addr_ovr = (acpi_madt_lapic_addr_ovr_t *)entry;
            if (entry->length < sizeof(acpi_madt_lapic_addr_ovr_t))
            {
                kpanic(NULL, "MADT LAPIC Address Override entry too small");
            }
            lapic_addr = addr_ovr->lapic_addr;
            log_early("MADT: Found LAPIC Address Override, addr=0x%lx",
                      addr_ovr->lapic_addr);
            break;
        }
        case MADT_ENTRY_LX2APIC: // Type 9: Local x2APIC
        {
            acpi_madt_lx2apic_t *lx2apic = (acpi_madt_lx2apic_t *)entry;
            if (entry->length < sizeof(acpi_madt_lx2apic_t))
            {
                kpanic(NULL, "MADT Local x2APIC entry too small");
            }
            if (madt_lx2apic_len < 256)
            {
                madt_lx2apic_list[madt_lx2apic_len++] = lx2apic;
            }
            log_early("MADT: Found Local x2APIC, x2apic_id=%u, acpi_id=%u, flags=0x%x",
                      lx2apic->x2apic_id, lx2apic->acpi_id, lx2apic->flags);
            break;
        }
        default:
        {
            log_early("warning: Unknown MADT entry type=%d", entry->type);
            break;
        }
        }

        offset += entry->length;
    }

    log_early("MADT parsed: %u LAPIC(s), %u IOAPIC(s), %u ISO(s), %u LAPIC NMI(s), %u IOAPIC NMI(s), %u x2APIC(s), LAPIC addr: 0x%lx",
              madt_lapic_len, madt_ioapic_len, madt_iso_len, madt_lapic_nmi_len,
              madt_ioapic_nmi_len, madt_lx2apic_len, lapic_addr);
}