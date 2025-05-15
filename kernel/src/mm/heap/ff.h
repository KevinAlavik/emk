/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef FF_H
#define FF_H

/* Kevin's simple First-Fit allocator */

#include <stdint.h>
#include <stddef.h>

#define FF_POOL_SIZE 512 // 2MB, in pages

typedef struct block
{
    size_t size;
    struct block *next;
} block_t;

#endif // FF_H