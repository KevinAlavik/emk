/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef SMP_H
#define SMP_H

#include <stdint.h>

extern uint32_t bootstrap_lapic_id;

typedef struct
{
    uint32_t lapic_id;
    uint32_t cpu_index;
} cpu_local_t;

void smp_init();
cpu_local_t *get_cpu_local(void);

#endif // SMP_H