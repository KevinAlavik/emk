/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <dev/pit.h>
#include <arch/io.h>
#include <sys/apic/lapic.h>
#include <sys/apic/ioapic.h>
#include <util/log.h>
#include <arch/smp.h>

#define PIT_VECTOR 32

void (*pit_callback)(struct register_ctx *ctx) = NULL;

void pit_handler(struct register_ctx *frame)
{
    if (pit_callback)
        pit_callback(frame);
    lapic_eoi();
}

void pit_init(idt_intr_handler handler)
{
    if (handler)
        pit_callback = handler;

    outb(0x43, 0x36);

    uint16_t divisor = 5966;
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    idt_register_handler(PIT_VECTOR, pit_handler);
    ioapic_map(0, PIT_VECTOR, 0, 0xFF); /* Broadcast to ALL CPUs */
}