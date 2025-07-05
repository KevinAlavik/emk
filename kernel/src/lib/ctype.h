/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef CTYPE_H
#define CTYPE_H

#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isspace(c)                                                             \
    ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\v' || (c) == '\f' || \
     (c) == '\r')

int atoi(const char* str);

#endif // CTYPE_H
