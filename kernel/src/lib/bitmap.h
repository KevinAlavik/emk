/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

static inline void bitmap_set(uint8_t *bitmap, uint64_t bit)
{
    bitmap[bit / 8] |= 1 << (bit % 8);
}

static inline void bitmap_clear(uint8_t *bitmap, uint64_t bit)
{
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline uint8_t bitmap_get(uint8_t *bitmap, uint64_t bit)
{
    return (bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

#endif // BITMAP_H