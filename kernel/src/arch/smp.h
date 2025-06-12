/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef SMP_H
#define SMP_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_CPUS                                                               \
    64 // eh, should be enough. We could increase to 256 but i doubt anyone
       // would run emk on that...

typedef struct {
    uint32_t lapic_id;
    uint32_t cpu_index;
    bool ready;
} cpu_local_t;

extern uint32_t bootstrap_lapic_id;
extern cpu_local_t cpu_locals[MAX_CPUS];
extern uint32_t cpu_count;

void smp_early_init(void);
void smp_init(void);
cpu_local_t* get_cpu_local(void);

#endif // SMP_H
