/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef IOAPIC_H
#define IOAPIC_H

#include <stdint.h>

#define IOAPIC_OFF_IOREGSEL 0x0
#define IOAPIC_OFF_IOWIN 0x10

#define IOAPIC_IDX_IOAPICID 0x00
#define IOAPIC_IDX_IOAPICVER 0x01
#define IOAPIC_IDX_RED_TBL 0x10

void ioapic_init();
void ioapic_map(int irq, int vec, uint8_t dest_mode, uint8_t lapic_id);
void ioapic_unmask(int irq);

#endif // IOAPIC_H