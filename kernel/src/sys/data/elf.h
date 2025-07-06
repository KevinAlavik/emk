/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef ELF_H
#define ELF_H

#include <stdint.h>

uint64_t elf_load(bool user, void* data, vctx_t* ctx);

#endif // ELF_H