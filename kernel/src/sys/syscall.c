/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <mm/vmm.h>
#include <sys/sched.h>
#include <sys/syscall.h>
#include <util/log.h>

int sys_exit(int code) {
    if (!sched_get_current())
        return -1; // TODO: Proper errno
    proc_exit(code);
    return 0;
}

int sys_test(void* buff) {
    vregion_t* r = vget(sched_get_current()->vctx, (uint64_t)buff);
    if (!r) {
        log("test(%p): region not found", buff);
        return -1;
    }

    log("test(%p): user=%s (flags=0x%x)", buff,
        (r->flags & VMM_USER) ? "Yes" : "No", r->flags);
    return 0;
}

syscall_fn_t syscall_table[] = {
    (syscall_fn_t)sys_exit, // SYS_exit
    (syscall_fn_t)sys_test,
};
