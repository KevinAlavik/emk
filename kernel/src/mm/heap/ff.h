/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef FF_H
#define FF_H

/* Kevin's simple First-Fit allocator */

#include <stddef.h>
#include <stdint.h>

#ifndef FF_POOL_SIZE
#define FF_POOL_SIZE 512
#endif // FF_POOL_SIZE

typedef struct block {
    size_t size;
    struct block* next;
} block_t;

#endif // FF_H
