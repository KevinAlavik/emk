/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/cpu.h>
#include <arch/idt.h>
#include <arch/smp.h>
#include <lib/string.h>
#include <stdarg.h>
#include <sys/apic/lapic.h>
#include <sys/kpanic.h>
#include <util/kprintf.h>
#include <util/log.h>

static const char* strings[32] = {"Division by Zero",
                                  "Debug",
                                  "Non-Maskable-Interrupt",
                                  "Breakpoint",
                                  "Overflow",
                                  "Bound Range Exceeded",
                                  "Invalid opcode",
                                  "Device (FPU) not available",
                                  "Double Fault",
                                  "RESERVED VECTOR",
                                  "Invalid TSS",
                                  "Segment not present",
                                  "Stack Segment Fault",
                                  "General Protection Fault",
                                  "Page Fault",
                                  "RESERVED VECTOR",
                                  "x87 FP Exception",
                                  "Alignment Check",
                                  "Machine Check (Internal Error)",
                                  "SIMD FP Exception",
                                  "Virtualization Exception",
                                  "Control  Protection Exception",
                                  "RESERVED VECTOR",
                                  "RESERVED VECTOR",
                                  "RESERVED VECTOR",
                                  "RESERVED VECTOR",
                                  "RESERVED VECTOR",
                                  "RESERVED VECTOR",
                                  "Hypervisor Injection Exception",
                                  "VMM Communication Exception",
                                  "Security Exception",
                                  "RESERVED VECTOR"};

/* TODO: Move to arch/ since its arch-specific */
static void capture_regs(struct register_ctx* context) {
    __asm__ volatile(
        "movq %%rax, %0\n\t"
        "movq %%rbx, %1\n\t"
        "movq %%rcx, %2\n\t"
        "movq %%rdx, %3\n\t"
        "movq %%rsi, %4\n\t"
        "movq %%rdi, %5\n\t"
        "movq %%rbp, %6\n\t"
        "movq %%r8,  %7\n\t"
        "movq %%r9,  %8\n\t"
        "movq %%r10, %9\n\t"
        "movq %%r11, %10\n\t"
        "movq %%r12, %11\n\t"
        "movq %%r13, %12\n\t"
        "movq %%r14, %13\n\t"
        "movq %%r15, %14\n\t"
        : "=m"(context->rax), "=m"(context->rbx), "=m"(context->rcx),
          "=m"(context->rdx), "=m"(context->rsi), "=m"(context->rdi),
          "=m"(context->rbp), "=m"(context->r9), "=m"(context->r9),
          "=m"(context->r10), "=m"(context->r11), "=m"(context->r12),
          "=m"(context->r13), "=m"(context->r14), "=m"(context->r15)
        :
        : "memory");

    __asm__ volatile("movq %%cs,  %0\n\t"
                     "movq %%ss,  %1\n\t"
                     "movq %%es,  %2\n\t"
                     "movq %%ds,  %3\n\t"
                     "movq %%cr0, %4\n\t"
                     "movq %%cr2, %5\n\t"
                     "movq %%cr3, %6\n\t"
                     "movq %%cr4, %7\n\t"
                     : "=r"(context->cs), "=r"(context->ss), "=r"(context->es),
                       "=r"(context->ds), "=r"(context->cr0),
                       "=r"(context->cr2), "=r"(context->cr3),
                       "=r"(context->cr4)
                     :
                     : "memory");

    __asm__ volatile("movq %%rsp, %0\n\t"
                     "pushfq\n\t"
                     "popq %1\n\t"
                     : "=r"(context->rsp), "=r"(context->rflags)
                     :
                     : "memory");

    context->rip = (uint64_t)__builtin_return_address(0);
}

void kpanic(struct register_ctx* ctx, const char* fmt, ...) {
    /* Halt all other CPU's using our custom die trap at 0xFE/254 */
    lapic_send_ipi(0, 0xFE, ICR_FIXED, ICR_PHYSICAL, ICR_ALL_EXCLUDING_SELF);

    /* Wait until all CPU's except the one that panicked is halted */
    for (uint32_t i = 0; i < cpu_count; i++) {
        cpu_local_t* cur = &cpu_locals[i];
        if (cur->cpu_index != get_cpu_local()->cpu_index) {
            while (cur->ready) {
                __asm__ volatile("pause");
            }
        }
    }

    struct register_ctx regs;

    if (ctx == NULL) {
        capture_regs(&regs);
        regs.err = 0xDEADBEEF;
        regs.vector = 0x0;
    } else {
        memcpy(&regs, ctx, sizeof(struct register_ctx));
    }

    char buf[1024];

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
    } else {
        if (regs.vector >= sizeof(strings) / sizeof(strings[0])) {
            snprintf(buf, sizeof(buf), "Unknown panic vector: %d", regs.vector);
        } else {
            snprintf(buf, sizeof(buf), "%s", strings[regs.vector]);
        }
    }

    cpu_local_t* cpu = get_cpu_local();
    if (cpu)
        log_panic("=== Kernel panic: '%s' on CPU %d @ 0x%.16llx ===", buf,
                  cpu->cpu_index, regs.rip);
    else
        log_panic("=== Kernel panic: '%s' on CPU ??? @ 0x%.16llx ===", buf,
                  regs.rip);
    log_panic("Registers:");
    log_panic(
        "  rax: 0x%.16llx  rbx:    0x%.16llx  rcx: 0x%.16llx  rdx: 0x%.16llx",
        regs.rax, regs.rbx, regs.rcx, regs.rdx);
    log_panic(
        "  rsi: 0x%.16llx  rdi:    0x%.16llx  rbp: 0x%.16llx  rsp: 0x%.16llx",
        regs.rsi, regs.rdi, regs.rbp, regs.rsp);
    log_panic(
        "  r8 : 0x%.16llx  r9 :    0x%.16llx  r10: 0x%.16llx  r11: 0x%.16llx",
        regs.r8, regs.r9, regs.r10, regs.r11);
    log_panic(
        "  r12: 0x%.16llx  r13:    0x%.16llx  r14: 0x%.16llx  r15: 0x%.16llx",
        regs.r12, regs.r13, regs.r14, regs.r15);
    log_panic("  rip: 0x%.16llx  rflags: 0x%.16llx", regs.rip, regs.rflags);
    log_panic(
        "  cs : 0x%.16llx  ss:     0x%.16llx  ds:  0x%.16llx  es:  0x%.16llx",
        regs.cs, regs.ss, regs.ds, regs.es);
    log_panic(
        "  cr0: 0x%.16llx  cr2:    0x%.16llx  cr3: 0x%.16llx  cr4: 0x%.16llx",
        regs.cr0, regs.cr2, regs.cr3, regs.cr4);
    log_panic("  err: 0x%.16llx  vector: 0x%.16llx", regs.err, regs.vector);

    hcf();
}