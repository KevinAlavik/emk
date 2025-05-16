/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef SMP_H
#define SMP_H

#include <stdint.h>

extern uint32_t bootstrap_lapic_id;
void smp_init();

#endif // SMP_H