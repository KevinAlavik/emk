/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef PIT_H
#define PIT_H

#include <arch/idt.h>

void pit_init(void (*callback)(struct register_ctx *ctx));

#endif // PIT_H