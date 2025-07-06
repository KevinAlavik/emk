#include <stdint.h>

static inline long syscall(uint64_t number, uint64_t arg1, uint64_t arg2,
                           uint64_t arg3) {
    long ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3)
                     : "memory");
    return ret;
}

void _start(void) {
    const char* test = "Hello, World!\n";
    syscall(1, (uint64_t)test, 0, 0);
    syscall(0, 0, 0, 0);
}