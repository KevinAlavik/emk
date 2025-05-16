/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef IOAPIC_H
#define IOAPIC_H

// Credits: https://github.com/SILD-Project/soaplin/blob/main/kernel/src/dev/ioapic.h

#include <stdbool.h>
#include <sys/acpi.h>
#include <sys/acpi/madt.h>

#define IOAPIC_REGSEL 0x0
#define IOAPIC_IOWIN 0x10

#define IOAPIC_ID 0x0
#define IOAPIC_VER 0x01
#define IOAPIC_ARB 0x02
#define IOAPIC_REDTBL 0x10

void ioapic_write(acpi_madt_ioapic_t *ioapic, uint8_t reg, uint32_t val);
uint32_t ioapic_read(acpi_madt_ioapic_t *ioapic, uint8_t reg);

void ioapic_redirect_irq(uint32_t lapic_id, uint8_t vec, uint8_t irq,
                         bool mask);
uint32_t ioapic_get_redirect_irq(uint8_t irq);

void ioapic_set_entry(acpi_madt_ioapic_t *ioapic, uint8_t idx, uint64_t data);

void ioapic_init();

#endif // IOAPIC_H