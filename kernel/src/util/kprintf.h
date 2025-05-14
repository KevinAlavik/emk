/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef KPRINTF_H
#define KPRINTF_H

#include <stddef.h>
#include <stdarg.h>

/* Minimal kprintf using nanoprintf, see external/nanoprintf.h */
int kprintf(const char *fmt, ...);

/* Careful with these */
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif // KPRINTF_H