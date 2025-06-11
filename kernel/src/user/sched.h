/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef SCHED_H
#define SCHED_H

#include <arch/idt.h>
#include <arch/paging.h>
#include <boot/emk.h>
#include <stdint.h>

typedef struct tcb tcb_t;
typedef struct pcb pcb_t;

typedef struct pcb {
    /* Meta data */
    const char* name;      // A name for the process, usually the process path
    uint32_t pid;          // Process ID tied to this proc
    __unused uint32_t uid; // Owner UID
    __unused uint32_t gid; // Owner GID

    /* Data */
    uint64_t* pm; // Root PML4 for the PCB

    tcb_t* root_thread; // The main thread
    pcb_t* next;        // Linked list :^)
} pcb_t;

typedef struct tcb {
    /* Meta data */
    const char name[32]; // A name for the thread, the first one usually has the
                         // same name as parent PCB, or "main"

    /* Data */
    struct register_ctx* ctx; // Pointer to register context
    pcb_t* parent;            // Pointer to parent PCB
    tcb_t* next;              // Linked list :^)
} tcb_t;

// TODO: Implement this sucker

#endif // SCHED_H
