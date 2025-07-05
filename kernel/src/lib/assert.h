/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef ASSERT_H
#define ASSERT_H

#include <arch/cpu.h>
#include <util/log.h>

#define assert(expr)                                                           \
    do {                                                                       \
        if (!(expr)) {                                                         \
            log_panic("Assertion failed: (%s), file: %s, line: %d", #expr,     \
                      __FILE__, __LINE__);                                     \
            hlt();                                                             \
        }                                                                      \
    } while (0)

#endif // ASSERT_H
