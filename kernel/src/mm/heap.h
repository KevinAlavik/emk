/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

void heap_init();
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif // HEAP_H