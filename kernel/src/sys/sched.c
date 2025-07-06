/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/cpu.h>
#include <arch/smp.h>
#include <lib/assert.h>
#include <lib/string.h>
#include <mm/heap.h>
#include <mm/pmm.h>
#include <sys/kpanic.h>
#include <sys/sched.h>
#include <sys/spinlock.h>
#include <util/log.h>

typedef struct {
    volatile uint32_t value;
} atomic_t;

#define ATOMIC_INIT(i) {(i)}

static inline uint32_t atomic_inc_fetch(atomic_t* atom) {
    uint32_t old;
    __asm__ volatile("lock; xaddl %0, %1"
                     : "=r"(old), "+m"(atom->value)
                     : "0"(1)
                     : "memory");

    return old;
}

#define PROC_DEFAULT_TIME 1
#define PROC_MAX_PROCS_PER_CPU 512

typedef struct {
    pcb_t** procs;
    uint32_t count;
    uint32_t current_pid;
    spinlock_t lock;
} cpu_sched_t;

static cpu_sched_t cpu_schedulers[MAX_CPUS];
static atomic_t global_pid_counter = ATOMIC_INIT(0);

void map_range_to_pagemap(uint64_t* dest_pagemap, uint64_t* src_pagemap,
                          uint64_t start, uint64_t size, uint64_t flags) {
    for (uint64_t offset = 0; offset < size; offset += PAGE_SIZE) {
        uint64_t phys = virt_to_phys(src_pagemap, start + offset);
        if (phys) {
            vmap(dest_pagemap, start + offset, phys, flags);
        }
    }
}

void sched_init(void) {
    cpu_local_t* cpu = get_cpu_local();
    cpu_sched_t* sched = &cpu_schedulers[cpu->cpu_index];

    sched->procs = (pcb_t**)kmalloc(sizeof(pcb_t*) * PROC_MAX_PROCS_PER_CPU);
    if (!sched->procs) {
        kpanic(NULL, "Failed to init sched proc list for CPU %d",
               cpu->cpu_index);
        return;
    }
    memset(sched->procs, 0, sizeof(pcb_t*) * PROC_MAX_PROCS_PER_CPU);
    sched->count = 0;
    sched->current_pid = 0;
    spinlock_init(&sched->lock);
}

uint32_t sched_spawn(bool user, void (*entry)(void), uint64_t* pagemap,
                     vctx_t* vctx) {
    cpu_local_t* cpu = get_cpu_local();
    cpu_sched_t* sched = &cpu_schedulers[cpu->cpu_index];

    spinlock_acquire(&sched->lock);

    if (sched->count >= PROC_MAX_PROCS_PER_CPU) {
        spinlock_release(&sched->lock);
        kpanic(NULL, "Maximum process limit reached for CPU %d",
               cpu->cpu_index);
        return 0;
    }

    pcb_t* proc = (pcb_t*)kmalloc(sizeof(pcb_t));
    if (!proc) {
        spinlock_release(&sched->lock);
        kpanic(NULL, "Failed to allocate memory for new proc");
        return 0;
    }

    memset(proc, 0, sizeof(pcb_t));
    proc->pid = atomic_inc_fetch(&global_pid_counter);
    proc->state = PROC_READY;
    proc->ctx.rip = (uint64_t)entry;
    proc->pagemap = pagemap ? pagemap : kernel_pagemap;
    proc->vctx = vctx ? vctx : vinit(proc->pagemap, 0x10000);

    uint64_t stack_size = 4; // 4 pages ~16KB
    uint64_t map_flags = VMM_PRESENT | VMM_WRITE;
    if (user) {
        map_flags |= VMM_USER;
        proc->ctx.cs = 0x1B;
        proc->ctx.ss = 0x23;
    } else {
        proc->ctx.cs = 0x08;
        proc->ctx.ss = 0x10;
    }

    void* stack = valloc(proc->vctx, stack_size, map_flags);
    if (!stack) {
        kfree(proc);
        sched->count--;
        spinlock_release(&sched->lock);
        kpanic(NULL, "Failed to allocate stack for new proc");
        return 0;
    }

    proc->ctx.rsp = (uint64_t)stack + (PAGE_SIZE * stack_size);
    proc->ctx.rflags = 0x202;

    vmap(proc->pagemap, (uint64_t)proc,
         virt_to_phys(kernel_pagemap, (uint64_t)proc), map_flags);
    map_range_to_pagemap(proc->pagemap, kernel_pagemap, (uint64_t)sched->procs,
                         sizeof(pcb_t*) * PROC_MAX_PROCS_PER_CPU, map_flags);

    map_range_to_pagemap(proc->pagemap, kernel_pagemap, 0x10000, 0x10000,
                         map_flags);

    proc->timeslice = PROC_DEFAULT_TIME;
    sched->procs[sched->count++] = proc;

    spinlock_release(&sched->lock);
    return proc->pid;
}

void sched_tick(struct register_ctx* ctx) {
    if (!ctx)
        return;

    cpu_local_t* cpu = get_cpu_local();
    cpu_sched_t* sched = &cpu_schedulers[cpu->cpu_index];

    spinlock_acquire(&sched->lock);
    if (sched->count == 0) {
        spinlock_release(&sched->lock);
        pmset(kernel_pagemap);
        return;
    }

    pcb_t* current_proc = sched->procs[sched->current_pid];
    if (current_proc && current_proc->state == PROC_RUNNING) {
        memcpy(&current_proc->ctx, ctx, sizeof(struct register_ctx));

        if (--current_proc->timeslice == 0) {
            current_proc->state = PROC_READY;
            current_proc->timeslice = PROC_DEFAULT_TIME;
        }
    }

    uint32_t start_pid = sched->current_pid;
    pcb_t* next_proc = NULL;
    do {
        sched->current_pid = (sched->current_pid + 1) % PROC_MAX_PROCS_PER_CPU;
        next_proc = sched->procs[sched->current_pid];

        if (next_proc && next_proc->state == PROC_TERMINATED) {
            vdestroy(next_proc->vctx);
            vunmap(next_proc->pagemap, (uint64_t)next_proc);
            kfree(next_proc);

            for (uint32_t i = sched->current_pid; i < sched->count - 1; i++) {
                sched->procs[i] = sched->procs[i + 1];
            }
            sched->procs[sched->count - 1] = NULL;
            sched->count--;
            next_proc = NULL;
        }
    } while (!next_proc && sched->current_pid != start_pid && sched->count > 0);

    if (!next_proc && sched->count > 0) {
        sched->current_pid = 0;
        next_proc = sched->procs[0];
    }

    if (next_proc && next_proc->state == PROC_READY) {
        next_proc->state = PROC_RUNNING;
        pmset(next_proc->pagemap);
        memcpy(ctx, &next_proc->ctx, sizeof(struct register_ctx));
    } else if (sched->count == 0) {
        log("No processes remaining on CPU %d. Halting.", cpu->cpu_index);
        spinlock_release(&sched->lock);
        pmset(kernel_pagemap);
        hlt();
    }

    spinlock_release(&sched->lock);
}

void sched_terminate(uint32_t pid) {
    cpu_local_t* cpu = get_cpu_local();
    cpu_sched_t* sched = &cpu_schedulers[cpu->cpu_index];

    spinlock_acquire(&sched->lock);
    for (uint32_t i = 0; i < sched->count; i++) {
        if (sched->procs[i] && sched->procs[i]->pid == pid) {
            sched->procs[i]->state = PROC_TERMINATED;
            log("pid %d exited with code %d", sched->procs[i]->pid,
                sched->procs[i]->exit_code);
            if (i == sched->current_pid) {
                struct register_ctx dummy_ctx;
                sched_tick(&dummy_ctx);
            }
            break;
        }
    }
    spinlock_release(&sched->lock);
}

void proc_exit(int32_t code) {
    cpu_local_t* cpu = get_cpu_local();
    cpu_sched_t* sched = &cpu_schedulers[cpu->cpu_index];

    spinlock_acquire(&sched->lock);

    pcb_t* current_proc = sched->procs[sched->current_pid];
    if (current_proc) {
        current_proc->exit_code = code;
        spinlock_release(&sched->lock);
        sched_terminate(current_proc->pid);
    }

    spinlock_release(&sched->lock);
}

pcb_t* sched_get_current(void) {
    cpu_local_t* cpu = get_cpu_local();
    cpu_sched_t* sched = &cpu_schedulers[cpu->cpu_index];
    return sched->procs[sched->current_pid];
}
