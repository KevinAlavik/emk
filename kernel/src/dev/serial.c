/* EMK 1.0 Copyright (c) 2025 Piraterna */
#include <arch/io.h>
#include <dev/serial.h>

int serial_init(uint16_t port) {
    uint16_t base = (uint16_t)port;

    // Disable all interrupts
    outb(base + UART_IER, 0x00);
    io_wait();

    // Enable DLAB to set baud rate
    outb(base + UART_LCR, UART_LCR_DLAB);
    io_wait();
    outb(base + UART_DLL, UART_BAUD_38400 & 0xFF);
    io_wait();
    outb(base + UART_DLH, (UART_BAUD_38400 >> 8) & 0xFF);
    io_wait();

    // Disable DLAB and set 8N1 (8 bits, no parity, 1 stop bit)
    outb(base + UART_LCR, UART_LCR_8N1);
    io_wait();

    // Enable FIFO, clear them, with 14-byte threshold
    outb(base + UART_FCR, UART_FCR_ENABLE | UART_FCR_CLEAR_RX |
                              UART_FCR_CLEAR_TX | UART_FCR_14BYTE);
    io_wait();

    // Set DTR, RTS, and OUT2
    outb(base + UART_MCR, UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2);
    io_wait();

    // Test serial chip (enable loopback mode)
    outb(base + UART_MCR,
         UART_MCR_LOOP | UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2);
    io_wait();
    outb(base + UART_DATA, 0xAE); // Send test byte
    io_wait();
    if (inb(base + UART_DATA) != 0xAE) {
        return -1;
    }

    outb(base + UART_MCR, UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2);
    io_wait();
    return 0;
}

int serial_write(uint16_t port, const uint8_t* data, uint32_t length) {
    uint16_t base = port;
    uint32_t i;

    for (i = 0; i < length; i++) {
        // Wait for transmitter holding register to be empty
        while (!(inb(base + UART_LSR) & UART_LSR_THRE)) {
            io_wait();
        }
        outb(base + UART_DATA, data[i]);
    }
    return i;
}

int serial_read(uint16_t port, uint8_t* buffer, uint32_t length) {
    uint16_t base = port;
    uint32_t i = 0;

    while (i < length) {
        if (inb(base + UART_LSR) & UART_LSR_DR) {
            buffer[i] = inb(base + UART_DATA);
            i++;
        } else {
            break;
        }
    }
    return i;
}