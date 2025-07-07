/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef USER_H
#define USER_H

#include <mm/vmm.h>
#include <stddef.h>

int copy_from_user(vctx_t* vctx, void* kdst, const void* usrc, size_t len);
int copy_to_user(vctx_t* vctx, void* user_dst, const void* kernel_src, size_t len);

#endif // USER_H