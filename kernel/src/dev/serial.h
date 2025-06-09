/* EMK 1.0 Copyright (c) 2025 Piraterna */
#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define COM1 0x3f8

// UART register offsets
#define UART_DATA 0x00 // Data register (R/W)
#define UART_IER 0x01  // Interrupt Enable Register (R/W)
#define UART_DLL 0x00  // Divisor Latch Low (R/W, when DLAB=1)
#define UART_DLH 0x01  // Divisor Latch High (R/W, when DLAB=1)
#define UART_FCR 0x02  // FIFO Control Register (W)
#define UART_LCR 0x03  // Line Control Register (R/W)
#define UART_MCR 0x04  // Modem Control Register (R/W)
#define UART_LSR 0x05  // Line Status Register (R)

// Line Control Register bits
#define UART_LCR_DLAB 0x80 // Divisor Latch Access Bit
#define UART_LCR_8N1 0x03  // 8 bits, no parity, 1 stop bit

// FIFO Control Register bits
#define UART_FCR_ENABLE 0x01   // Enable FIFO
#define UART_FCR_CLEAR_RX 0x02 // Clear receive FIFO
#define UART_FCR_CLEAR_TX 0x04 // Clear transmit FIFO
#define UART_FCR_14BYTE 0xC0   // 14-byte trigger level

// Modem Control Register bits
#define UART_MCR_DTR 0x01  // Data Terminal Ready
#define UART_MCR_RTS 0x02  // Request to Send
#define UART_MCR_OUT2 0x08 // OUT2 (used for interrupts)
#define UART_MCR_LOOP 0x10 // Loopback mode

// Line Status Register bits
#define UART_LSR_DR 0x01   // Data Ready
#define UART_LSR_THRE 0x20 // Transmitter Holding Register Empty

// Baud rate divisor for 38400 baud (115200 / 3)
#define UART_BAUD_38400 3

int serial_init(uint16_t port);
int serial_write(uint16_t port, const uint8_t* data, uint32_t length);
int serial_read(uint16_t port, uint8_t* buffer, uint32_t length);

#endif // SERIAL_H