/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef IOAPIC_H
#define IOAPIC_H

#include <arch/idt.h>

// I/O APIC Registers
#define IOAPIC_OFF_IOREGSEL 0x0
#define IOAPIC_OFF_IOWIN 0x10
#define IOAPIC_IDX_IOAPICVER 0x01

void ioapic_init();
void ioapic_map(int irq, int vec, idt_intr_handler handler);

#endif // IOAPIC_H