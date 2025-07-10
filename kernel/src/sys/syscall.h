#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

enum { SYS_exit = 0, SYS_kping, SYSCALL_TABLE_SIZE };

typedef int (*syscall_fn_t)(uintptr_t, uintptr_t, uintptr_t);

long syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2,
                      uint64_t arg3);

#define SYSCALL_TO_STR(n)                                                      \
    ((n) == SYS_exit ? "exit" : (n) == SYS_kping ? "kping" : "unknown")

static inline long syscall(uint64_t num, uint64_t arg1, uint64_t arg2,
                           uint64_t arg3) {
    long ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3)
                     : "memory");
    return ret;
}

#endif // SYSCALL_H
