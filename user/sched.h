/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef SCHED_H
#define SCHED_H

#include <arch/idt.h>
#include <boot/emk.h>
#include <mm/vmm.h>
#include <stdint.h>

#define TFLAG_USER BIT(0)

typedef enum {
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_TERMINATED
} task_state_t;

typedef struct tcb {
    struct register_ctx regs;
    task_state_t state;
    uint32_t tid;
    uint32_t priority;
    uint8_t flags;
    uint64_t* pagemap;
    vctx_t* vctx;
    struct tcb* next;
} tcb_t;

void sched_init();
int tcreate(void (*entry)(void), uint8_t flags);
void schedule(struct register_ctx* ctx);
tcb_t* get_current_task(void);
void task_exit(void);

#endif // SCHED_H