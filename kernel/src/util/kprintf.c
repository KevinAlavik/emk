/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <dev/serial.h>
#include <util/kprintf.h>

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#define NANOPRINTF_IMPLEMENTATION
#include <nanoprintf.h>

int kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    int length = npf_vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (length >= 0 && length < (int)sizeof(buffer))
    {
        serial_write(COM1, (uint8_t *)buffer, length);
    }

    va_end(args);
    return length;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int length = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return length;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    int length = npf_vsnprintf(buf, size, fmt, args);

    if (length >= (int)size)
    {
        buf[size - 1] = '\0';
    }

    return length;
}