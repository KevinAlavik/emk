/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/smp.h>
#include <arch/cpu.h>
#include <boot/limine.h>
#include <boot/emk.h>
#include <util/log.h>
#include <sys/kpanic.h>
#include <stdatomic.h>
#include <lib/string.h>
#include <mm/heap.h>
#include <arch/io.h>
#include <sys/apic/lapic.h>
#include <sys/acpi/madt.h>
#include <arch/paging.h>
#include <util/align.h>
#include <mm/pmm.h>
#include <arch/gdt.h>
#include <arch/idt.h>

#define MAX_CPUS 256
#define MSR_GS_BASE 0xC0000101

uint32_t cpu_count = 0;
uint32_t bootstrap_lapic_id = 0;
atomic_uint started_cpus = 0;
static cpu_local_t cpu_locals[MAX_CPUS];

cpu_local_t *get_cpu_local(void)
{
    return (cpu_local_t *)rdmsr(MSR_GS_BASE);
}

static inline void set_cpu_local(cpu_local_t *cpu)
{
    wrmsr(MSR_GS_BASE, (uint64_t)cpu);
}

void smp_entry(struct limine_mp_info *smp_info)
{
    uint32_t lapic_id = smp_info->lapic_id;
    cpu_local_t *cpu = NULL;

    for (uint32_t i = 0; i < cpu_count; i++)
    {
        if (cpu_locals[i].lapic_id == lapic_id)
        {
            cpu = &cpu_locals[i];
            break;
        }
    }

    if (!cpu)
        kpanic(NULL, "CPU with LAPIC ID %u not found!", lapic_id);

    set_cpu_local(cpu);

    /* Setup core */
    gdt_init();
    idt_init();
    pmset(kernel_pagemap);
    lapic_enable();

    atomic_fetch_add(&started_cpus, 1);
    log_early("CPU %d (LAPIC ID %u) is up", cpu->cpu_index, lapic_id);

    cpu->ready = true;
    hlt();
}

void smp_init(void)
{
    bootstrap_lapic_id = mp_response->bsp_lapic_id;
    cpu_count = mp_response->cpu_count;
    log_early("%u CPUs detected", cpu_count);

    lapic_enable();

    for (uint32_t i = 0; i < cpu_count; i++)
    {
        struct limine_mp_info *info = mp_response->cpus[i];
        memset(&cpu_locals[i], 0, sizeof(cpu_local_t));
        cpu_locals[i].lapic_id = info->lapic_id;
        cpu_locals[i].cpu_index = i;
        cpu_locals[i].ready = false;

        if (info->lapic_id == bootstrap_lapic_id)
        {
            set_cpu_local(&cpu_locals[i]);
            cpu_locals[i].ready = true;
            atomic_fetch_add(&started_cpus, 1);
            log_early("CPU %u (LAPIC ID %u) is the bootstrap processor", i, info->lapic_id);
        }
        else
        {
            __atomic_store_n(&info->goto_address, smp_entry, __ATOMIC_SEQ_CST);

            volatile uint64_t timeout = 0;
            while (!cpu_locals[i].ready)
            {
                __asm__ volatile("pause");
                timeout++;
            }

            if (!cpu_locals[i].ready)
            {
                log_early("warning: CPU %u (LAPIC ID %u) failed to start", i, info->lapic_id);
            }
            else
            {
                log_early("CPU %u (LAPIC ID %u) started successfully", i, info->lapic_id);
            }
        }
    }

    log_early("All CPUs are ready");
}