/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/smp.h>
#include <boot/limine.h>
#include <boot/emk.h>
#include <util/log.h>

uint32_t bootstrap_lapic_id = 0;
uint32_t cpu_count = 0;

uint32_t ctr = 0;

void smp_entry(struct limine_mp_info *smp_info)
{

    log_early("CPU %d started", smp_info->processor_id);
    __atomic_fetch_add(&ctr, 1, __ATOMIC_SEQ_CST);

    while (1)
        ;
}

void smp_init()
{
    bootstrap_lapic_id = mp_response->bsp_lapic_id;
    cpu_count = mp_response->cpu_count;

    for (uint64_t i = 0; i < cpu_count; i++)
    {
        if (mp_response->cpus[i]->lapic_id != bootstrap_lapic_id)
        {
            uint32_t old_ctr = __atomic_load_n(&ctr, __ATOMIC_SEQ_CST);

            __atomic_store_n(&mp_response->cpus[i]->goto_address, smp_entry,
                             __ATOMIC_SEQ_CST);

            while (__atomic_load_n(&ctr, __ATOMIC_SEQ_CST) == old_ctr)
                ;
        }
        else
        {
            log_early("CPU %d is the bootstrap processor", i);
        }
    }
}