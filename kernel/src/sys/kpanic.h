#ifndef KPANIC_H
#define KPANIC_H

#include <arch/idt.h>

void kpanic(struct register_ctx *ctx, const char *fmt, ...);

#endif // KPANIC_H