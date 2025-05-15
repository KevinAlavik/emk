/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/acpi.h>
#include <boot/emk.h>
#include <lib/string.h>
#include <util/log.h>
#include <sys/kpanic.h>

static int acpi_uses_xsdt = 0;
static void *acpi_rsdt_ptr = NULL;

static uint8_t acpi_checksum(void *ptr, size_t len)
{
    uint8_t sum = 0;
    uint8_t *data = (uint8_t *)ptr;
    for (size_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}

static int acpi_validate_rsdp(acpi_rsdp_t *rsdp)
{
    return acpi_checksum(rsdp, sizeof(acpi_rsdp_t)) == 0;
}

static int acpi_validate_xsdp(acpi_xsdp_t *xsdp)
{
    if (acpi_checksum(xsdp, sizeof(acpi_rsdp_t)) != 0)
        return 0;
    if (acpi_checksum(xsdp, xsdp->length) != 0)
        return 0;
    return 1;
}

void acpi_init(void)
{
    if (!rsdp_response || !rsdp_response->address)
    {
        kpanic(NULL, "No RSDP provided");
    }

    acpi_rsdp_t *rsdp = (acpi_rsdp_t *)rsdp_response->address;
    log_early("RSDP found at %p", rsdp);
    if (strncmp(rsdp->signature, "RSD PTR ", 8) != 0)
    {
        kpanic(NULL, "Invalid RSDP signature");
    }

    if (rsdp->revision >= 2)
    {
        acpi_xsdp_t *xsdp = (acpi_xsdp_t *)rsdp;
        if (!acpi_validate_xsdp(xsdp))
        {
            kpanic(NULL, "XSDP checksum validation failed");
        }
        acpi_uses_xsdt = 1;
        acpi_rsdt_ptr = (void *)(uintptr_t)xsdp->xsdt_address;
    }
    else
    {
        if (!acpi_validate_rsdp(rsdp))
        {
            kpanic(NULL, "RSDP checksum validation failed");
        }
        acpi_uses_xsdt = 0;
        acpi_rsdt_ptr = (void *)(uintptr_t)rsdp->rsdt_address;
    }

    /* Validate RSDT/XSDT header */
    acpi_sdt_header_t *sdt = (acpi_sdt_header_t *)acpi_rsdt_ptr;
    if (acpi_checksum(sdt, sdt->length) != 0)
    {
        kpanic(NULL, "RSDT/XSDT checksum validation failed");
    }

    log_early("ACPI initialized, using %s at %p",
              acpi_uses_xsdt ? "XSDT" : "RSDT", acpi_rsdt_ptr);
}

void *acpi_find_table(const char *name)
{
    if (!acpi_rsdt_ptr || !name)
        return NULL;

    acpi_sdt_header_t *sdt = (acpi_sdt_header_t *)acpi_rsdt_ptr;
    uint32_t entries;

    if (acpi_uses_xsdt)
    {
        entries = (sdt->length - sizeof(acpi_sdt_header_t)) / sizeof(uint64_t);
    }
    else
    {
        entries = (sdt->length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t);
    }

    for (uint32_t i = 0; i < entries; i++)
    {
        acpi_sdt_header_t *table;
        if (acpi_uses_xsdt)
        {
            uint64_t *xsdt_entries = (uint64_t *)(sdt + 1);
            table = (acpi_sdt_header_t *)(uintptr_t)xsdt_entries[i];
        }
        else
        {
            uint32_t *rsdt_entries = (uint32_t *)(sdt + 1);
            table = (acpi_sdt_header_t *)(uintptr_t)rsdt_entries[i];
        }

        if (strncmp(table->signature, name, 4) == 0)
        {
            if (acpi_checksum(table, table->length) == 0)
            {
                return table;
            }
        }
    }

    return NULL;
}