#include <boot/limine.h>
#include <arch/cpu.h>
#include <arch/io.h>
#include <dev/serial.h>

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);
__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

void emk_entry(void)
{
    if (serial_init(COM1) != 0)
    {
        /* Just halt and say nothing */
        hcf();
    }

    if (!LIMINE_BASE_REVISION_SUPPORTED)
    {
        serial_write(COM1, (uint8_t *)"ERROR: Limine base revision is not supported\n", 45);
        hcf();
    }

    serial_write(COM1, (uint8_t *)"Hello, World!\n", 14);
    hlt();
}