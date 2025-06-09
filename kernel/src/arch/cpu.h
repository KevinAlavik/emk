/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdnoreturn.h>

static inline noreturn void hlt() {
    for (;;)
        __asm__ volatile("hlt");
}

static inline noreturn void hcf() {
    __asm__ volatile("cli");
    for (;;)
        __asm__ volatile("hlt");
}

static inline void wrmsr(uint64_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t rdmsr(uint64_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

#endif // CPU_H