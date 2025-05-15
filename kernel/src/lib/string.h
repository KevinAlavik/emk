/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stddef.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

int strncmp(const char *s1, const char *s2, size_t n);

#endif // STRING_H