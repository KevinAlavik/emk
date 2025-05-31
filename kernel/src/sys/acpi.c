#include <sys/acpi.h>
#include <boot/emk.h>
#include <lib/string.h>
#include <util/log.h>
#include <sys/kpanic.h>
#include <mm/vmm.h>
#include <mm/heap.h>

static int acpi_uses_xsdt = 0;
static void *acpi_rsdt_ptr = NULL;

void *acpi_vtable(uint64_t phys)
{
    acpi_sdt_header_t *tmp = (acpi_sdt_header_t *)vallocat(kvm_ctx, 1, VALLOC_RW, phys);
    if (!tmp)
    {
        kpanic(NULL, "Failed to map first page of ACPI table");
        return NULL;
    }

    uint32_t full_size = tmp->length;
    uint32_t page_count = (full_size + 0xFFF) / 0x1000;
    return vallocat(kvm_ctx, page_count, VALLOC_RW, phys);
}

void acpi_init(void)
{
    if (!rsdp_response || !rsdp_response->address)
    {
        kpanic(NULL, "No RSDP provided");
    }

    acpi_rsdp_t *rsdp = (acpi_rsdp_t *)vallocat(kvm_ctx, 1, VALLOC_RW, rsdp_response->address);
    if (memcmp(rsdp->signature, "RSD PTR", 7) != 0)
        kpanic(NULL, "Invalid RSDP signature! EMK depends on APIC 1.0 or higher");

    if (rsdp->revision >= 2)
    {
        acpi_uses_xsdt = 1;
        acpi_xsdp_t *xsdp = (acpi_xsdp_t *)rsdp;
        acpi_rsdt_ptr = acpi_vtable(xsdp->xsdt_address);
        log_early("Using XSDT");
    }
    else
    {
        acpi_uses_xsdt = 0;
        acpi_rsdt_ptr = acpi_vtable(rsdp->rsdt_address);
        log_early("Using RSDT");
    }
}

void *acpi_find_table(const char *name)
{
    if (!acpi_rsdt_ptr || !name)
        return NULL;

    if (acpi_uses_xsdt)
    {
        acpi_xsdt_t *xsdt = (acpi_xsdt_t *)acpi_rsdt_ptr;
        uint32_t entries = (xsdt->sdt.length - sizeof(xsdt->sdt)) / 8;

        for (uint32_t i = 0; i < entries; i++)
        {
            uint64_t phys_addr = ((uint64_t *)xsdt->table)[i];
            acpi_sdt_header_t *sdt = (acpi_sdt_header_t *)acpi_vtable(phys_addr);
            if (!memcmp(sdt->signature, name, 4))
                return sdt;
        }
    }
    else
    {
        acpi_rsdt_t *rsdt = (acpi_rsdt_t *)acpi_rsdt_ptr;
        uint32_t entries = (rsdt->sdt.length - sizeof(rsdt->sdt)) / 4;

        for (uint32_t i = 0; i < entries; i++)
        {
            uint32_t phys_addr = ((uint32_t *)rsdt->table)[i];
            acpi_sdt_header_t *sdt = (acpi_sdt_header_t *)acpi_vtable(phys_addr);
            if (!memcmp(sdt->signature, name, 4))
                return sdt;
        }
    }

    return NULL;
}
