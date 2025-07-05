/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <lib/ctype.h>
#include <lib/string.h>

int atoi(const char* str) {
    if (!str)
        return 0;

    int result = 0;
    int sign = 1;

    while (isspace(*str))
        str++;

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (isdigit(*str)) {
        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}
