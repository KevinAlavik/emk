/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/cpu.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/io.h>
#include <arch/paging.h>
#include <arch/smp.h>
#include <boot/emk.h>
#include <boot/limine.h>
#include <lib/string.h>
#include <mm/heap.h>
#include <mm/pmm.h>
#include <stdatomic.h>
#include <sys/acpi.h>
#include <sys/acpi/madt.h>
#include <sys/apic/ioapic.h>
#include <sys/apic/lapic.h>
#include <sys/kpanic.h>
#include <util/align.h>
#include <util/log.h>

#define MSR_GS_BASE 0xC0000101
#define CPU_START_TIMEOUT 10000000

uint32_t cpu_count = 0;
uint32_t bootstrap_lapic_id = 0;
atomic_uint started_cpus = 0;
cpu_local_t cpu_locals[MAX_CPUS] = {0};

cpu_local_t* get_cpu_local(void) {
    cpu_local_t* cpu = (cpu_local_t*)rdmsr(MSR_GS_BASE);
    if (cpu) {
        return cpu;
    }

    uint32_t current_lapic_id = lapic_get_id();
    for (uint32_t i = 0; i < cpu_count; i++) {
        if (cpu_locals[i].lapic_id == current_lapic_id) {
            return &cpu_locals[i];
        }
    }

    log_early("Error: No CPU found with LAPIC ID %u", current_lapic_id);
    return NULL;
}

static inline void set_cpu_local(cpu_local_t* cpu) {
    wrmsr(MSR_GS_BASE, (uint64_t)cpu);
}

static void init_cpu(cpu_local_t* cpu) {
    (void)cpu;
    // TODO: CPU-specific GDT with different TSS and such.
    gdt_init();

    // FIXME: Maybe not re-initialize the entire IDT but rather just reload it
    idt_init();

    pmset(kernel_pagemap);
    lapic_enable();
    tss_init(kstack_top);
}

void smp_entry(struct limine_mp_info* smp_info) {
    uint32_t lapic_id = smp_info->lapic_id;
    cpu_local_t* cpu = NULL;

    for (uint32_t i = 0; i < cpu_count; i++) {
        if (cpu_locals[i].lapic_id == lapic_id) {
            cpu = &cpu_locals[i];
            break;
        }
    }

    if (!cpu) {
        kpanic(NULL, "CPU with LAPIC ID %u not found", lapic_id);
    }

    set_cpu_local(cpu);
    init_cpu(cpu);

    cpu->ready = true;
    atomic_fetch_add(&started_cpus, 1);
    __asm__ volatile("sti");
    hlt();
}

void smp_early_init(void) {
    if (!mp_response) {
        kpanic(NULL, "No MP response from Limine");
    }

    cpu_count = mp_response->cpu_count;
    if (cpu_count > MAX_CPUS)
        kpanic(
            NULL,
            "You have too many cores man, the max we support is %d, not %d >:D",
            MAX_CPUS, cpu_count);
    bootstrap_lapic_id = mp_response->bsp_lapic_id;
    log_early("Detected %u CPUs, BSP LAPIC ID: %u", cpu_count,
              bootstrap_lapic_id);

    for (uint32_t i = 0; i < cpu_count; i++) {
        struct limine_mp_info* info = mp_response->cpus[i];
        cpu_locals[i].lapic_id = info->lapic_id;
        cpu_locals[i].cpu_index = i;
        cpu_locals[i].ready = false;
    }
}

void smp_init(void) {
    if (!mp_response) {
        kpanic(NULL, "No MP response from Limine");
    }

    lapic_enable();

    for (uint32_t i = 0; i < cpu_count; i++) {
        struct limine_mp_info* info = mp_response->cpus[i];
        if (info->lapic_id == bootstrap_lapic_id) {
            set_cpu_local(&cpu_locals[i]);
            init_cpu(&cpu_locals[i]);
            cpu_locals[i].ready = true;
            atomic_fetch_add(&started_cpus, 1);
        } else {
            __atomic_store_n(&info->goto_address, smp_entry, __ATOMIC_SEQ_CST);
            uint64_t timeout = 0;
            while (!cpu_locals[i].ready && timeout < CPU_START_TIMEOUT) {
                __asm__ volatile("pause");
                timeout++;
            }
            if (!cpu_locals[i].ready) {
                kpanic(NULL,
                       "CPU %u (LAPIC ID %u) failed to start within timeout: "
                       "%u of %u started",
                       i, info->lapic_id, atomic_load(&started_cpus),
                       cpu_count);
            }
        }
    }

    if (atomic_load(&started_cpus) != cpu_count) {
        kpanic(NULL, "Not all CPUs started: %u of %u started",
               atomic_load(&started_cpus), cpu_count);
    }
}
