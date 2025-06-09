/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/io.h>
#include <arch/smp.h>
#include <dev/pit.h>
#include <sys/apic/ioapic.h>
#include <sys/apic/lapic.h>
#include <util/log.h>

#define PIT_VECTOR 32

void (*pit_callback)(struct register_ctx* ctx) = NULL;

void pit_handler(struct register_ctx* frame) {
    if (pit_callback)
        pit_callback(frame);
    lapic_eoi();
}

#if ENABLE_PIT
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
#else
void pit_init(idt_intr_handler handler) {
    (void)handler;
    log_early("warning: Tried to initialize PIT: PIT is a deprecated feature "
              "of the kernel, but you can force enable it in menuconfig");
}
#endif // ENABLE_PIT
