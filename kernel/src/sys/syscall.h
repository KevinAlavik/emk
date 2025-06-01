/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

/* TODO: Implement syscalls */
static inline long
syscall(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    long ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3)
        : "memory");
    return ret;
}

#endif // SYSCALL_H