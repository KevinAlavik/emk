/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef ASSERT_H
#define ASSERT_H

#include <arch/cpu.h>
#include <arch/smp.h>
#include <sys/sched.h>
#include <util/log.h>

#define assert(expr)                                                           \
    do {                                                                       \
        if (!(expr)) {                                                         \
            pcb_t* p = sched_get_current();                                    \
            if (p)                                                             \
                log_panic(                                                     \
                    "Assertion failed: (%s), file: %s, line: %d, CPU: %d, "    \
                    "PID: %d",                                                 \
                    #expr, __FILE__, __LINE__, get_cpu_local()->cpu_index,     \
                    p->pid);                                                   \
            else                                                               \
                log_panic(                                                     \
                    "Assertion failed: (%s), file: %s, line: %d, CPU: %d, "    \
                    "PID: %d",                                                 \
                    #expr, __FILE__, __LINE__, -1);                            \
            hlt();                                                             \
        }                                                                      \
    } while (0)

#endif // ASSERT_H
