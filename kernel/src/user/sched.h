/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef SCHED_H
#define SCHED_H

// Based on:
// https://codeberg.org/sild/soaplin/src/commit/ca602d4f73ea35003199defe40e7547dc12b5727

#include <arch/idt.h>
#include <arch/paging.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <stdint.h>

#define THREAD_NOT_INITIALIZED 0
#define THREAD_READY 1
#define THREAD_RUNNING 2
#define THREAD_BLOCKED 3
#define THREAD_SLEEPING 4

struct process;

typedef struct thread {
    uint64_t stack_base;
    struct register_ctx regs;
    int tid;
    int state;
    bool user;
    struct process* parent;
    struct thread* next;
    struct thread* list_next;
} thread_t;

typedef struct process {
    int pid;
    uint64_t* pm;
    vctx_t* vma;

    struct process* next;
    struct thread* threads;
    struct thread* threads_tail;
} process_t;

void sched_init();
process_t* sched_new();
thread_t* sched_new_thread(process_t* parent);
void schedule(struct register_ctx* regs);
process_t* sched_spawn(void (*f)(), bool user);

#endif // SCHED_H
