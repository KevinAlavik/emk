/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/paging.h>
#include <lib/string.h>
#include <mm/heap.h>
#include <mm/vmm.h>
#include <sys/kpanic.h>
#include <user/sched.h>
#include <util/log.h>

static tcb_t* ready_queue = NULL;
static tcb_t* current_task = NULL;
static uint32_t next_tid = 1;

static void enqueue_task(tcb_t* task) {
    if (!task)
        return;

    task->state = TASK_STATE_READY;
    task->next = NULL;

    if (!ready_queue) {
        ready_queue = task;
    } else {
        tcb_t* curr = ready_queue;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = task;
    }
}

static tcb_t* dequeue_task(void) {
    if (!ready_queue)
        return NULL;

    tcb_t* task = ready_queue;
    ready_queue = task->next;
    task->next = NULL;
    return task;
}
void idle(void) {
    // idle tast
    while (1)
        ;
}

void sched_init(void) {
    tcb_t* init_task = (tcb_t*)kmalloc(sizeof(tcb_t));
    if (!init_task) {
        kpanic(NULL, "Failed to allocate memory for initial task");
        return;
    }
    memset(init_task, 0, sizeof(tcb_t));

    init_task->regs.cs = 0x08;                   /* Kernel code segment */
    init_task->regs.ss = 0x10;                   /* Kernel data segment */
    init_task->regs.rflags = 0x202;              /* Interrupts enabled */
    init_task->regs.rsp = (uint64_t)&kstack_top; /* Use kernel stack */
    init_task->regs.rip = (uint64_t)idle;
    init_task->tid = next_tid++;
    init_task->state = TASK_STATE_RUNNING;
    init_task->priority = 0; /* Highest priority */
    init_task->flags = 0;    /* Kernel task */
    init_task->next = NULL;
    init_task->pagemap = kernel_pagemap;

    init_task->vctx = kvm_ctx;
    if (!init_task->vctx) {
        kfree(init_task);
        kpanic(NULL, "Failed to initialize virtual context for init_task");
        return;
    }

    current_task = init_task;
}

int tcreate(void (*entry)(void), uint8_t flags) {
    tcb_t* task = (tcb_t*)kmalloc(sizeof(tcb_t));
    if (!task)
        return -1;

    /* Allocate stack (8 pages = 32KB) */
    size_t stack_pages = 8;
    vctx_t* vctx = vinit(pmnew(), VPM_MIN_ADDR);
    if (!vctx) {
        kfree(task);
        return -1;
    }

    uint64_t stack_flags = VALLOC_RW | (flags & TFLAG_USER ? VALLOC_USER : 0);
    void* stack_top = valloc(vctx, stack_pages, stack_flags);
    if (!stack_top) {
        vdestroy(vctx);
        kfree(task);
        return -1;
    }

    memset(task, 0, sizeof(tcb_t));
    task->vctx = vctx;
    task->tid = next_tid++;
    task->priority = 0;
    task->state = TASK_STATE_READY;
    task->flags = flags;
    task->next = NULL;

    task->regs.rip = (uint64_t)entry;
    task->regs.rsp = (uint64_t)stack_top + (stack_pages * 4096);
    task->regs.rflags = 0x202; /* Interrupts enabled */
    if (flags & TFLAG_USER) {
        task->regs.cs = 0x1B; /* User code segment (GDT + 3) */
        task->regs.ss = 0x23; /* User data segment (GDT + 3) */
        task->pagemap = pmnew();
    } else {
        task->regs.cs = 0x08; /* Kernel code segment */
        task->regs.ss = 0x10; /* Kernel data segment */
        task->pagemap = kernel_pagemap;
    }

    enqueue_task(task);
    return task->tid;
}

static void context_switch(struct register_ctx* ctx, tcb_t* current,
                           tcb_t* next) {
    if (!current || !next) {
        return;
    }

    memcpy(&current->regs, ctx, sizeof(struct register_ctx));
    pmset(next->pagemap);
    memcpy(ctx, &next->regs, sizeof(struct register_ctx));
}

void schedule(struct register_ctx* ctx) {
    if (!ready_queue) {
        if (current_task && current_task->state == TASK_STATE_RUNNING) {
            return;
        }
        log("warning: No more tasks available");
        return;
    }

    tcb_t* next_task = dequeue_task();
    if (!next_task) {
        return;
    }

    if (current_task && current_task->state == TASK_STATE_RUNNING) {
        current_task->state = TASK_STATE_READY;
        enqueue_task(current_task);
    }

    next_task->state = TASK_STATE_RUNNING;
    context_switch(ctx, current_task, next_task);
    current_task = next_task;
}

tcb_t* get_current_task(void) { return current_task; }

void task_exit(void) {
    if (!current_task)
        return;

    uint32_t tid = current_task->tid;
    current_task->state = TASK_STATE_TERMINATED;
    vdestroy(current_task->vctx);
    kfree(current_task);
    current_task = NULL;
    log("tid %d exited", tid);
    struct register_ctx dummy_ctx;
    schedule(&dummy_ctx);
}