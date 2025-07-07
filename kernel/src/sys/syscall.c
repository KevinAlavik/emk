/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <lib/string.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/sched.h>
#include <sys/syscall.h>
#include <util/log.h>

static inline size_t min_size(size_t a, size_t b) { return (a < b) ? a : b; }

int copy_from_user(vctx_t* vctx, void* kdst, const void* usrc, size_t len) {
    size_t copied = 0;
    uint8_t* dst = (uint8_t*)kdst;
    uint64_t src_addr = (uint64_t)usrc;

    while (copied < len) {
        vregion_t* r = vget(vctx, src_addr);
        if (!r) {
            log("copy_from_user: region not found for %p", (void*)src_addr);
            return -1;
        }

        if (!(r->flags & VMM_USER)) {
            log("copy_from_user: region flags invalid at %p: 0x%x (%s)",
                (void*)src_addr, r->flags, vpflags_to_str(r->flags));
            return -1;
        }

        size_t page_offset = src_addr & (PAGE_SIZE - 1);
        size_t chunk = min_size(len - copied, PAGE_SIZE - page_offset);

        void* user_ptr = (void*)src_addr;

        log("copy_from_user: copying %zu bytes from %p (flags=0x%x)", chunk,
            user_ptr, r->flags);

        memcpy(dst + copied, user_ptr, chunk);

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
        if (!r) {
            log("copy_to_user: region not found for %p", (void*)dst_addr);
            return -1;
        }

        if (!(r->flags & VMM_USER) || !(r->flags & VMM_WRITE)) {
            log("copy_to_user: region flags invalid at %p: 0x%x (%s)",
                (void*)dst_addr, r->flags, vpflags_to_str(r->flags));
            return -1;
        }

        size_t page_offset = dst_addr & (PAGE_SIZE - 1);
        size_t chunk = min_size(len - copied, PAGE_SIZE - page_offset);

        void* user_ptr = (void*)dst_addr;

        log("copy_to_user: writing %zu bytes to %p (flags=0x%x)", chunk,
            user_ptr, r->flags);

        memcpy(user_ptr, src + copied, chunk);

        copied += chunk;
        dst_addr += chunk;
    }

    return 0;
}

int sys_exit(int code) {
    if (!sched_get_current())
        return -1; // TODO: Proper errno
    proc_exit(code);
    return 0;
}

int sys_msg(char* s) {
    log("Message from PID %d: \033[1m%s\033[0m", sched_get_current()->pid, s);
    return 0;
}

syscall_fn_t syscall_table[] = {
    (syscall_fn_t)sys_exit, // SYS_exit
    (syscall_fn_t)sys_msg,  // SYS_msg
};
