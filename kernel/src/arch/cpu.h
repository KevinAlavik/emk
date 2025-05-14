#ifndef CPU_H
#define CPU_H

#include <stdnoreturn.h>

static inline noreturn void hlt()
{
    for (;;)
        __asm__ volatile("hlt");
}

static inline noreturn void hcf()
{
    __asm__ volatile("cli");
    for (;;)
        __asm__ volatile("hlt");
}

#endif // CPU_H