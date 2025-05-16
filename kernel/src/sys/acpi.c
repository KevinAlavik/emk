/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/acpi.h>
#include <boot/emk.h>
#include <lib/string.h>
#include <util/log.h>
#include <sys/kpanic.h>

static int acpi_uses_xsdt = 0;
static void *acpi_rsdt_ptr = NULL;

void acpi_init(void)
{
    if (!rsdp_response || !rsdp_response->address)
    {
        kpanic(NULL, "No RSDP provided");
    }

    acpi_rsdp_t *rsdp = (acpi_rsdp_t *)HIGHER_HALF(rsdp_response->address);
    if (memcmp(rsdp->signature, "RSD PTR", 7))
        kpanic(NULL, "Invalid RSDP signature!");

    if (rsdp->revision != 0)
    {
        acpi_uses_xsdt = 1;
        acpi_xsdp_t *xsdp = (acpi_xsdp_t *)rsdp;
        acpi_rsdt_ptr = (acpi_xsdt_t *)HIGHER_HALF(xsdp->xsdt_address);
        log_early("XSDT found at %p", acpi_rsdt_ptr);
        return;
    }

    acpi_rsdt_ptr = (acpi_rsdt_t *)HIGHER_HALF(rsdp->rsdt_address);
    log_early("RSDT found at %p", acpi_rsdt_ptr);
}

void *acpi_find_table(const char *name)
{
    if (!acpi_uses_xsdt)
    {
        acpi_rsdt_t *rsdt = (acpi_rsdt_t *)acpi_rsdt_ptr;
        uint32_t entries = (rsdt->sdt.length - sizeof(rsdt->sdt)) / 4;
        for (uint32_t i = 0; i < entries; i++)
        {
            acpi_sdt_header_t *sdt = (acpi_sdt_header_t *)HIGHER_HALF(*((uint32_t *)rsdt->table + i));
            if (!memcmp(sdt->signature, name, 4))
                return (void *)sdt;
        }
        return NULL;
    }
    acpi_xsdt_t *xsdt = (acpi_xsdt_t *)acpi_rsdt_ptr;
    log_early("XSDT address: %p", xsdt);
    uint32_t entries = (xsdt->sdt.length - sizeof(xsdt->sdt)) / 8;

    for (uint32_t i = 0; i < entries; i++)
    {
        acpi_sdt_header_t *sdt = (acpi_sdt_header_t *)HIGHER_HALF(*((uint64_t *)xsdt->table + i));
        if (!memcmp(sdt->signature, name, 4))
        {
            return (void *)sdt;
        }
    }

    return NULL;
}