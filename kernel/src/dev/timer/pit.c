/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/io.h>
#include <arch/smp.h>
#include <dev/timer/pit.h>
#include <sys/apic/ioapic.h>
#include <sys/apic/lapic.h>
#include <util/log.h>

#define PIT_VECTOR 32
static bool _tapi_enabled = false;

void (*pit_callback)(struct register_ctx* ctx) = NULL;

void pit_handler(struct register_ctx* frame) {
    if (pit_callback)
        pit_callback(frame);
    lapic_eoi();
}

void pit_init(idt_intr_handler handler) {
    if (handler)
        pit_callback = handler;

    outb(0x43, 0x36);

    uint16_t divisor = 5966;
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    idt_register_handler(PIT_VECTOR, pit_handler);
#if BROADCAST_PIT
    ioapic_map(0, PIT_VECTOR, 0, 0xFF); /* Broadcast to ALL CPUs */
#else
    ioapic_map(0, PIT_VECTOR, 0, get_cpu_local()->lapic_id);
#endif // BROADCAST_PIT
}

// Exposed timer API
void timer_init(idt_intr_handler handler) {
    pit_init(handler);
    _tapi_enabled = true;
}

bool timer_enabled() { return _tapi_enabled; }
