/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef PIT_H
#define PIT_H

#include <arch/idt.h>

void pit_init(idt_intr_handler handler);

#endif // PIT_H