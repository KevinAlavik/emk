/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <sys/lapic.h>
#include <boot/emk.h>

uint64_t apic_ticks = 0;

void lapic_init()
{
    lapic_write(0xf0, 0x1ff);
}

void lapic_stop_timer()
{
    lapic_write(LAPIC_TIMER_INITCNT, 0);
    lapic_write(LAPIC_TIMER_LVT, LAPIC_TIMER_DISABLE);
}

void lapic_oneshot(uint8_t vec, uint64_t ms)
{
    lapic_stop_timer();
    lapic_write(LAPIC_TIMER_DIV, 0);
    lapic_write(LAPIC_TIMER_LVT, vec);
    lapic_write(LAPIC_TIMER_INITCNT, apic_ticks * ms);
}

void lapic_calibrate_timer()
{
    lapic_stop_timer();
    lapic_write(LAPIC_TIMER_DIV, 0);
    lapic_write(LAPIC_TIMER_LVT, (1 << 16) | 0xff);
    lapic_write(LAPIC_TIMER_INITCNT, 0xFFFFFFFF);
    lapic_write(LAPIC_TIMER_LVT, LAPIC_TIMER_DISABLE);
    uint32_t ticks = 0xFFFFFFFF - lapic_read(LAPIC_TIMER_CURCNT);
    apic_ticks = ticks;
    lapic_stop_timer();
}

void lapic_write(uint32_t reg, uint32_t val)
{
    *((volatile uint32_t *)(HIGHER_HALF(0xfee00000) + reg)) = val;
}

uint32_t lapic_read(uint32_t reg)
{
    return *((volatile uint32_t *)(HIGHER_HALF(0xfee00000) + reg));
}

void lapic_eoi() { lapic_write((uint8_t)0xb0, 0x0); }

void lapic_ipi(uint32_t id, uint8_t dat)
{
    lapic_write(LAPIC_ICRHI, id << LAPIC_ICDESTSHIFT);
    lapic_write(LAPIC_ICRLO, dat);
}

void lapic_send_all_int(uint32_t id, uint32_t vec)
{
    lapic_ipi(id, vec | LAPIC_ICRAIS);
}

void lapic_send_others_int(uint32_t id, uint32_t vec)
{
    lapic_ipi(id, vec | LAPIC_ICRAES);
}

uint32_t lapic_get_id() { return lapic_read(0x0020) >> LAPIC_ICDESTSHIFT; }