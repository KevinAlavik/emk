/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/sched.h>
#include <sys/syscall.h>
#include <util/log.h>

int sys_exit(int code) {
    log("syscall: exit(%d)", code);
    if (!sched_get_current())
        return -1; // TODO: Proper errno
    proc_exit(code);
    return 0;
}

syscall_fn_t syscall_table[] = {
    (syscall_fn_t)sys_exit, // SYS_exit
};
