/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef SCHED_H
#define SCHED_H

#include <arch/idt.h>
#include <arch/paging.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <stdint.h>

#define PROC_DEFAULT_TIME 1
#define PROC_MAX_PROCS 2048

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_WAITING,
    PROC_TERMINATED,
} proc_state_t;

typedef struct pcb {
    uint32_t pid;
    proc_state_t state;
    uint64_t timeslice;
    uint64_t* pagemap;
    vctx_t* vctx;
    struct register_ctx ctx;
    int32_t exit_code;
} pcb_t;

void sched_init();
uint32_t sched_spawn(bool user, void (*entry)(void), uint64_t* pagemap,
                     vctx_t* vctx);
void sched_tick(struct register_ctx* ctx);
pcb_t* sched_get_current();
void proc_exit(int32_t code);

#endif // SCHED_H
