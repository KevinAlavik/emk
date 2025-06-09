/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef ALIGN_H
#define ALIGN_H

#define DIV_ROUND_UP(x, y)                                                     \
    (((uint64_t)(x) + ((uint64_t)(y) - 1)) / (uint64_t)(y))
#define ALIGN_UP(x, y) (DIV_ROUND_UP(x, y) * (uint64_t)(y))
#define ALIGN_DOWN(x, y) (((uint64_t)(x) / (uint64_t)(y)) * (uint64_t)(y))

#define IS_PAGE_ALIGNED(x) (((uintptr_t)(x) & (PAGE_SIZE - 1)) == 0)

#endif // ALIGN_H