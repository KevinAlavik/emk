/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/cpu.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/smp.h>
#include <lib/string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/kpanic.h>
#include <sys/sched.h>
#include <sys/syscall.h>
#include <util/errno.h>
#include <util/log.h>

struct idt_entry __attribute__((aligned(16))) idt_descriptor[256] = {0};
idt_intr_handler real_handlers[256] = {0};
extern uint64_t stubs[];

struct __attribute__((packed)) idt_ptr {
    uint16_t limit;
    uint64_t base;
};

struct idt_ptr idt_ptr = {sizeof(idt_descriptor) - 1,
                          (uint64_t)&idt_descriptor};

void syscall_handler(struct register_ctx* ctx) {
    long status = syscall_dispatch(ctx->rax, ctx->rdi, ctx->rsi, ctx->rdx);
    pcb_t* proc = sched_get_current();
    if (proc && status < 0) {
        proc->errno = -status;
        log("pid %d: %s(): %s", proc->pid, SYSCALL_TO_STR(ctx->rax),
            ERRNO_TO_STR(proc->errno));
    }

    ctx->rax = status;
}

void idt_default_interrupt_handler(struct register_ctx* ctx) {
    kpanic(ctx, NULL);
}

/* pesky little trap which just halts the current cpu and lets it die alone */
void die(struct register_ctx* ctx) {
    (void)ctx;

    /* If the CPU has caught this its game over */
    cpu_local_t* cpu = get_cpu_local();
    cpu->ready = false;
    hcf();
}

void idt_set_gate(uint8_t interrupt, uint64_t base, uint8_t flags) {
    idt_descriptor[(interrupt)].off_low = (base) & 0xFFFF;
    idt_descriptor[(interrupt)].sel = 0x8;
    idt_descriptor[(interrupt)].ist = 0;
    idt_descriptor[(interrupt)].attr = (flags);
    idt_descriptor[(interrupt)].off_mid = ((base) >> 16) & 0xFFFF;
    idt_descriptor[(interrupt)].off_high = ((base) >> 32) & 0xFFFFFFFF;
    idt_descriptor[(interrupt)].zero = 0;
}

void idt_init() {
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, stubs[i], IDT_TRAP_GATE);
        real_handlers[i] = idt_default_interrupt_handler;
    }

    for (int i = 32; i < 256; i++) {
        idt_set_gate(i, stubs[i], IDT_INTERRUPT_GATE);
    }

    idt_set_gate(0x80, stubs[0x80], IDT_INTERRUPT_GATE | GDT_ACCESS_RING3);
    real_handlers[0x80] = syscall_handler;

    idt_set_gate(0xFE, stubs[0xFE], IDT_TRAP_GATE);
    real_handlers[0xFE] = die;

    __asm__ volatile("lidt %0" : : "m"(idt_ptr) : "memory");
}

int idt_register_handler(size_t vector, idt_intr_handler handler) {
    if (vector >= 256 || handler == NULL)
        return 1;

    real_handlers[vector] = handler;
    return 0;
}
