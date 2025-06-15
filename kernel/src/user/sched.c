/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <boot/emk.h>
#include <lib/string.h>
#include <sys/kpanic.h>
#include <sys/spinlock.h>
#include <user/sched.h>
#include <util/align.h>

static spinlock_t sched_lock = {0};

static process_t* proc_list_head;
static process_t* proc_list_tail;

static thread_t* thread_list_head;
static thread_t* thread_list_tail;

process_t* current_proc;
thread_t* current_thread;

static int numpid = -1;
static int numtid = -1;

static bool sched_ready = false;

static void _sched_add_thread_to_process(process_t* parent, thread_t* new) {
    if (!parent || !new)
        return;

    if (parent->threads) {
        thread_t* t = parent->threads;
        while (t->next)
            t = t->next;
        t->next = new;
    } else {
        parent->threads = new;
    }
}

void sched_init() {
    spinlock_acquire(&sched_lock);

    if (sched_ready) {
        spinlock_release(&sched_lock);
        return;
    }

    process_t* sys = sched_new();
    if (!sys) {
        spinlock_release(&sched_lock);
        kpanic(NULL, "sched_init: failed to create system process");
    }

    pfree(sys->pm, 1);
    sys->pm = kernel_pagemap;

    thread_t* th = sched_new_thread(sys);
    if (!th) {
        spinlock_release(&sched_lock);
        kpanic(NULL, "sched_init: failed to create system thread");
    }

    th->state = THREAD_RUNNING;
    th->stack_base = kstack_top;
    th->user = false;

    current_proc = sys;
    current_thread = th;
    sched_ready = true;

    spinlock_release(&sched_lock);
}

process_t* sched_new() {
    spinlock_acquire(&sched_lock);

    process_t* new = valloc(kvm_ctx, ALIGN_UP(sizeof(process_t), PAGE_SIZE),
                            VMM_PRESENT | VMM_WRITE);
    if (!new) {
        spinlock_release(&sched_lock);
        return NULL;
    }

    memset(new, 0, sizeof(process_t));
    new->pid = ++numpid;
    new->pm = pmnew();
    new->vma = vinit(new->pm, 0x10000);

    if (proc_list_head)
        proc_list_tail->next = new;
    else
        proc_list_head = new;

    proc_list_tail = new;

    spinlock_release(&sched_lock);
    return new;
}

thread_t* sched_new_thread(process_t* parent) {
    if (!sched_ready || !parent)
        return NULL;

    spinlock_acquire(&sched_lock);

    thread_t* new = valloc(kvm_ctx, ALIGN_UP(sizeof(thread_t), PAGE_SIZE),
                           VMM_PRESENT | VMM_WRITE);
    if (!new) {
        spinlock_release(&sched_lock);
        return NULL;
    }

    memset(new, 0, sizeof(thread_t));
    new->tid = ++numtid;
    new->parent = parent;
    new->user = true;
    new->state = THREAD_READY;

    _sched_add_thread_to_process(parent, new);

    if (!thread_list_head) {
        thread_list_head = new;
        thread_list_tail = new;
        new->list_next = new;
    } else {
        thread_list_tail->list_next = new;
        new->list_next = thread_list_head;
        thread_list_tail = new;
    }

    spinlock_release(&sched_lock);
    return new;
}

thread_t* sched_select_thread() {
    if (!sched_ready || !current_thread) {
        kpanic(NULL,
               "sched_select_thread: scheduler not ready or no current thread");
    }

    if (current_thread->list_next == current_thread) {
        return current_thread;
    }

    thread_t* t = current_thread->list_next;
    while (t != current_thread) {
        if (t->state == THREAD_RUNNING)
            return t;
        t = t->list_next;
    }

    kpanic(NULL, "sched_select_thread: No runnable threads found");
    __builtin_unreachable();
}

void schedule(struct register_ctx* regs) {
    if (!current_thread || !regs) {
        kpanic(NULL, "schedule: scheduler not initialized or invalid state");
    }

    spinlock_acquire(&sched_lock);

    memcpy(&current_thread->regs, regs, sizeof(struct register_ctx));

    current_thread = sched_select_thread();
    current_proc = current_thread->parent;

    memcpy(regs, &current_thread->regs, sizeof(struct register_ctx));
    pmset(current_proc->pm);

    spinlock_release(&sched_lock);
}

process_t* sched_spawn(void (*f)(), bool user) {
    process_t* p = sched_new();
    if (!p)
        kpanic(NULL, "sched_spawn: failed to create new process");

    thread_t* t = sched_new_thread(p);
    if (!t)
        kpanic(NULL, "sched_spawn: failed to create new thread");

    if (user) {
        t->stack_base =
            (uint64_t)valloc(p->vma, 8, VMM_PRESENT | VMM_USER | VMM_WRITE);
        if (!t->stack_base)
            kpanic(NULL, "sched_spawn: failed to allocate user stack");

        t->regs.rsp = t->stack_base + (8 * PAGE_SIZE);
        t->regs.cs = 0x1b; // user mode code segment
        t->regs.ss = 0x23; // user mode stack segment
    } else {
        t->stack_base = (uint64_t)valloc(p->vma, 8, VMM_PRESENT | VMM_WRITE);
        if (!t->stack_base)
            kpanic(NULL, "sched_spawn: failed to allocate user stack");
        t->regs.rsp = t->stack_base + (8 * PAGE_SIZE);
        t->regs.cs = 0x08; // kernel mode code segment
        t->regs.ss = 0x10; // kernel mode stack segment
    }
    t->regs.rip = (uint64_t)f;
    t->regs.rflags = 0x202;

    t->state = THREAD_RUNNING;
    return p;
}
