/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/idt.h>
#include <lib/string.h>
#include <stdarg.h>
#include <arch/cpu.h>
#include <util/log.h>
#include <sys/kpanic.h>
#include <arch/smp.h>
#include <stdbool.h>

struct idt_entry __attribute__((aligned(16))) idt_descriptor[256] = {0};
idt_intr_handler real_handlers[256] = {0};
extern uint64_t stubs[];

struct __attribute__((packed)) idt_ptr
{
    uint16_t limit;
    uint64_t base;
};

struct idt_ptr idt_ptr = {sizeof(idt_descriptor) - 1, (uint64_t)&idt_descriptor};

void idt_default_interrupt_handler(struct register_ctx *ctx)
{
    kpanic(ctx, NULL);
}

#define SET_GATE(interrupt, base, flags)                                    \
    do                                                                      \
    {                                                                       \
        idt_descriptor[(interrupt)].off_low = (base) & 0xFFFF;              \
        idt_descriptor[(interrupt)].sel = 0x8;                              \
        idt_descriptor[(interrupt)].ist = 0;                                \
        idt_descriptor[(interrupt)].attr = (flags);                         \
        idt_descriptor[(interrupt)].off_mid = ((base) >> 16) & 0xFFFF;      \
        idt_descriptor[(interrupt)].off_high = ((base) >> 32) & 0xFFFFFFFF; \
        idt_descriptor[(interrupt)].zero = 0;                               \
    } while (0)

void idt_init()
{
    for (int i = 0; i < 32; i++)
    {
        SET_GATE(i, stubs[i], IDT_TRAP_GATE);
        real_handlers[i] = idt_default_interrupt_handler;
    }

    for (int i = 32; i < 256; i++)
    {
        SET_GATE(i, stubs[i], IDT_INTERRUPT_GATE);
    }

    __asm__ volatile(
        "lidt %0"
        : : "m"(idt_ptr) : "memory");
}

int idt_register_handler(size_t vector, idt_intr_handler handler)
{
    if (vector >= 256 || handler == NULL)
        return 1;

    real_handlers[vector] = handler;
    return 0;
}