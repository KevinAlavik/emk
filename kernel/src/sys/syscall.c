/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <boot/emk.h>
#include <lib/string.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/sched.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <util/errno.h>
#include <util/log.h>

static int sys_exit(uintptr_t code, __unused uintptr_t unused1,
                    __unused uintptr_t unused2) {
    if (!sched_get_current())
        return -ESRCH;
    proc_exit((int)code);
    return 0;
}

static syscall_fn_t syscall_table[] = {
    [SYS_exit] = sys_exit,
};

long syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2,
                      uint64_t arg3) {
    if (num >= SYSCALL_TABLE_SIZE || !syscall_table[num])
        return -ENOSYS;
    return syscall_table[num](arg1, arg2, arg3);
}
