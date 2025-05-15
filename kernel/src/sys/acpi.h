/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef EMK_ACPI_H
#define EMK_ACPI_H

#include <stdint.h>
#include <stddef.h>
#include <lib/string.h>

typedef struct acpi_rsdp
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed)) acpi_rsdp_t;

typedef struct acpi_xsdp
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t full_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) acpi_xsdp_t;

typedef struct acpi_sdt_header
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

void *acpi_find_table(const char *name);
void acpi_init(void);

#endif /* EMK_ACPI_H */