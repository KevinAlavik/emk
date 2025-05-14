/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef LOG_H
#define LOG_H

#include <util/kprintf.h>

#define log_early(fmt, ...) kprintf("early: " fmt "\n", ##__VA_ARGS__)
#define log_panic(fmt, ...) kprintf("panic: " fmt "\n", ##__VA_ARGS__)

#endif // LOG_H