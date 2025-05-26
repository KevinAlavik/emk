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

#define MAX_CPUS 256
#define MSR_GS_BASE 0xC0000101

uint32_t cpu_count = 0;
uint32_t bootstrap_lapic_id = 0;

atomic_uint started_cpus = 0;
static cpu_local_t cpu_locals[MAX_CPUS];

cpu_local_t *get_cpu_local(void)
{
    cpu_local_t *cpu = (cpu_local_t *)rdmsr(MSR_GS_BASE);
    return cpu;
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
        kpanic(NULL, "CPU with LAPIC ID %u not found in cpu_locals!", lapic_id);

    set_cpu_local(cpu);
    atomic_fetch_add(&started_cpus, 1);

    log_early("CPU %d is up", cpu->cpu_index);

    cpu->ready = true;
    while (1)
        __asm__ volatile("hlt");
}

void smp_init(void)
{
    bootstrap_lapic_id = mp_response->bsp_lapic_id;
    cpu_count = mp_response->cpu_count;

    log_early("%u CPUs detected", cpu_count);

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
            log_early("CPU %u is the bootstrap processor", i);
            atomic_fetch_add(&started_cpus, 1);
            cpu_locals[i].ready = true;
        }
        else
        {
            __atomic_store_n(&info->goto_address, smp_entry, __ATOMIC_SEQ_CST);
            while (atomic_load(&started_cpus) < (i + 1))
                __asm__ volatile("pause");
        }
    }

    bool all_ready = false;
    while (!all_ready)
    {
        all_ready = true;
        for (uint32_t i = 0; i < cpu_count; i++)
        {
            if (!cpu_locals[i].ready)
            {
                all_ready = false;
                __asm__ volatile("pause");
                break;
            }
        }
    }

    log_early("All CPUs are ready");
}
