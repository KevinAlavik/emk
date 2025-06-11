/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef TIMER_H
#define TIMER_H

#include <arch/idt.h>
#include <stdbool.h>

// Implemented in dev/timer/*.c
void timer_init(idt_intr_handler handler);
bool timer_enabled();

#endif // TIMER_H
