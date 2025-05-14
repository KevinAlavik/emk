#ifndef LOG_H
#define LOG_H

#include <util/kprintf.h>

#define early(fmt, ...) kprintf("early: " fmt "\n", ##__VA_ARGS__)

#endif // LOG_H