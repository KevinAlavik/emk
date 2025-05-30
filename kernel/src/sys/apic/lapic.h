/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef LAPIC_H
#define LAPIC_H

#include <stdint.h>

#define LAPIC_BASE 0x1b
#define LAPIC_REGOFF_LAPICID 0x20
#define LAPIC_REGOFF_EOI 0xB0
#define LAPIC_REGOFF_SPIV 0xF0

uint32_t lapic_read(uint32_t offset);
void lapic_write(uint32_t offset, uint32_t value);
void lapic_init();

#endif // LAPIC_H