/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef LAPIC_H
#define LAPIC_H

#include <stdint.h>

#define LAPIC_BASE_MSR 0x1B

// Local APIC Registers
#define LAPIC_ID 0x0020    // Local APIC ID
#define LAPIC_VER 0x0030   // Local APIC Version
#define LAPIC_TPR 0x0080   // Task Priority
#define LAPIC_EOI 0x00B0   // End of Interrupt
#define LAPIC_SVR 0x00F0   // Spurious Interrupt Vector
#define LAPIC_ESR 0x0280   // Error Status
#define LAPIC_ICRLO 0x0300 // Interrupt Command (Low)
#define LAPIC_ICRHI 0x0310 // Interrupt Command (High)
#define LAPIC_TIMER 0x0320 // LVT Timer
#define LAPIC_TICR 0x0380  // Timer Initial Count
#define LAPIC_TDCR 0x03E0  // Timer Divide Configuration

// ICR Fields
#define ICR_FIXED 0x00000000
#define ICR_INIT 0x00000500
#define ICR_STARTUP 0x00000600
#define ICR_PHYSICAL 0x00000000
#define ICR_ASSERT 0x00004000
#define ICR_EDGE 0x00000000
#define ICR_NO_SHORTHAND 0x00000000
#define ICR_SEND_PENDING 0x00001000
#define ICR_DESTINATION_SHIFT 24

// Startup vector address (must be in first 1MB, page-aligned)
#define AP_BOOT_ADDRESS 0x8000 // Example: 32KB physical address

extern uint64_t lapic_addr;

uint32_t lapic_read(uint32_t offset);
void lapic_write(uint32_t offset, uint32_t value);
void lapic_init(void);
void lapic_enable(void);
void lapic_eoi(void);

#endif // LAPIC_H