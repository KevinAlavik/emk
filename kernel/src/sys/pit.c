/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/pit.h>
#include <arch/io.h>
#include <sys/lapic.h>

void (*pit_callback)(struct register_ctx *ctx) = NULL;

void pit_handler(struct register_ctx *frame)
{
    if (pit_callback)
        pit_callback(frame);
    lapic_eoi();
}

void pit_init(void (*callback)(struct register_ctx *ctx))
{
    if (callback)
        pit_callback = callback;
    outb(0x43, 0x36);
    idt_register_handler(IDT_IRQ_BASE + 0, pit_handler);
    uint16_t divisor = 5966; // ~200Hz
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}