/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/paging.h>
#include <lib/string.h>
#include <mm/pmm.h>
#include <stdint.h>
#include <sys/user.h>

static inline size_t min_size(size_t a, size_t b) { return (a < b) ? a : b; }

int copy_from_user(vctx_t* vctx, void* kdst, const void* usrc, size_t len) {
    size_t copied = 0;
    uint8_t* dst = (uint8_t*)kdst;
    uint64_t src_addr = (uint64_t)usrc;

    while (copied < len) {
        vregion_t* r = vget(vctx, src_addr);
        if (!r || !(r->flags & VMM_USER))
            return -1;

        size_t page_offset = src_addr & (PAGE_SIZE - 1);
        size_t chunk = min_size(len - copied, PAGE_SIZE - page_offset);
        memcpy(dst + copied, (void*)src_addr, chunk);

        copied += chunk;
        src_addr += chunk;
    }

    return 0;
}

int copy_to_user(vctx_t* vctx, void* user_dst, const void* kernel_src,
                 size_t len) {
    size_t copied = 0;
    uint64_t dst_addr = (uint64_t)user_dst;
    const uint8_t* src = (const uint8_t*)kernel_src;

    while (copied < len) {
        vregion_t* r = vget(vctx, dst_addr);
        if (!r || !(r->flags & VMM_USER) || !(r->flags & VMM_WRITE))
            return -1;

        size_t page_offset = dst_addr & (PAGE_SIZE - 1);
        size_t chunk = min_size(len - copied, PAGE_SIZE - page_offset);
        memcpy((void*)dst_addr, src + copied, chunk);

        copied += chunk;
        dst_addr += chunk;
    }

    return 0;
}