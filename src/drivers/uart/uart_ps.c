/*
 * ======================================================================================
 * COPYRIGHT (C) 2026 PHOTONX TECHNOLOGIES. ALL RIGHTS RESERVED.
 * ======================================================================================
 * File:        uart_ps.c
 * Module:      Cadence UART Driver Implementation
 * Platform:    Xilinx Zynq UltraScale+ MPSoC
 * ======================================================================================
 */

#include "drivers/uart_ps.h"
#include "drivers/gic_v2.h"
#include "kernel/timer_heavy.h"

/* Primary Console Instance (UART0 or UART1 based on board config) */
/* On Kria KV260, UART1 is usually the USB-UART */
uart_driver_t console_uart = {
    .base_addr = ZYNQMP_UART1_BASE, 
    .baud_rate = 115200,
    .irq_num   = 54 // SPI 22 + 32 = 54 for UART1
};

/* Register Access Macros */
#define UART_READ(reg)          (*(volatile uint32_t *)(console_uart.base_addr + reg))
#define UART_WRITE(reg, val)    (*(volatile uint32_t *)(console_uart.base_addr + reg) = (val))

/*
 * ======================================================================================
 * RING BUFFER UTILITIES
 * ======================================================================================
 * We use circular buffers to allow the Kernel to write thousands of logs
 * without waiting for the slow serial port to physically send each byte.
 */

static void rb_push(ring_buffer_t *rb, uint8_t data) {
    uint32_t next = (rb->head + 1) % UART_RING_BUFFER_SIZE;
    
    /* If buffer is full, overwrite the oldest data (Drop strategy)
     * In a critical system, we might want to block or panic here.
     */
    if (next == rb->tail) {
        rb->tail = (rb->tail + 1) % UART_RING_BUFFER_SIZE;
        /* TODO: Increment overflow counter */
    }

    rb->buffer[rb->head] = data;
    rb->head = next;
    rb->count++;
}

static int rb_pop(ring_buffer_t *rb, uint8_t *data) {
    if (rb->head == rb->tail) {
        return 0; // Empty
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % UART_RING_BUFFER_SIZE;
    rb->count--;
    return 1; // Success
}
/*
 * ======================================================================================
 * INITIALIZATION & CONFIGURATION
 * ======================================================================================
 */

/*
 * uart_calc_baud_divisors
 * Calculates the Baud Rate Generator (CD) and Baud Rate Divider (BDIV)
 * based on the Input Clock (Sel_Clk) and Target Baud Rate.
 * * Formula: Baud_Rate = Sel_Clk / (CD * (BDIV + 1))
 */
static void uart_calc_baud_divisors(uint32_t target_baud, uint32_t *cd, uint32_t *bdiv) {
    uint32_t input_clk = 100000000; // Assume 100MHz (LPD_UART_CLK)
    uint32_t best_error = 0xFFFFFFFF;
    uint32_t calc_baud;
    uint32_t error;
    uint32_t best_cd = 0;
    uint32_t best_bdiv = 0;

    /* Iterate through all possible BDIV values (4 to 255) */
    for (uint32_t i = 4; i < 255; i++) {
        /* Calculate ideal CD */
        uint32_t tmp_cd = input_clk / (target_baud * (i + 1));
        
        /* Check constraints (CD must be 1-65535) */
        if (tmp_cd < 1 || tmp_cd > 65535) continue;

        /* Calculate resulting error */
        calc_baud = input_clk / (tmp_cd * (i + 1));
        error = (calc_baud > target_baud) ? (calc_baud - target_baud) : (target_baud - calc_baud);

        if (error < best_error) {
            best_error = error;
            best_cd = tmp_cd;
            best_bdiv = i;
        }
    }

    *cd = best_cd;
    *bdiv = best_bdiv;
}

void uart_init_controller(void) {
    /* 1. Disable UART (TX and RX) */
    UART_WRITE(UART_CR_OFFSET, UART_CR_TX_DIS | UART_CR_RX_DIS);

    /* 2. Configure Mode Register */
    /* 8 Data bits, No Parity, 1 Stop bit */
    UART_WRITE(UART_MR_OFFSET, UART_MR_CHARLEN_8 | UART_MR_PAR_NONE | UART_MR_NBSTOP_1);

    /* 3. Configure Baud Rate */
    uint32_t cd, bdiv;
    uart_calc_baud_divisors(console_uart.baud_rate, &cd, &bdiv);
    
    UART_WRITE(UART_BAUDGEN_OFFSET, cd);   // CD
    UART_WRITE(UART_BAUDDIV_OFFSET, bdiv); // BDIV

    /* 4. Reset FIFOs */
    UART_WRITE(UART_CR_OFFSET, UART_CR_TXRST | UART_CR_RXRST);
    
    /* Spin wait for reset to complete */
    volatile int delay = 1000;
    while(delay--);

    /* 5. Set Trigger Levels */
    UART_WRITE(UART_RXWM_OFFSET, 1);  // Trigger IRQ on 1 byte received
    UART_WRITE(UART_TXWM_OFFSET, 32); // Trigger IRQ when TX buffer is half empty

    /* 6. Enable UART (TX and RX) */
    UART_WRITE(UART_CR_OFFSET, UART_CR_TX_EN | UART_CR_RX_EN | UART_CR_TORST);

    /* 7. Initialize Ring Buffers */
    console_uart.tx_buffer.head = 0;
    console_uart.tx_buffer.tail = 0;
    console_uart.rx_buffer.head = 0;
    console_uart.rx_buffer.tail = 0;

    /* 8. Enable Interrupts (Optional - Polled mode preferred for early boot) */
    /* UART_WRITE(UART_IER_OFFSET, UART_IXR_RXOVR | UART_IXR_RXFULL | UART_IXR_TXEMPTY); */
    
    /* Mark as active */
    uart_send_string("\n[UART] Controller Initialized Successfully.\n");
}
/*
 * ======================================================================================
 * DATA TRANSMISSION & RECEPTION
 * ======================================================================================
 */

int uart_is_busy(void) {
    return !(UART_READ(UART_SR_OFFSET) & UART_SR_TXEMPTY);
}

void uart_send_byte(uint8_t c) {
    /*
     * Blocking Mode for Safety:
     * Wait until the Hardware FIFO is not full (TNFUL).
     */
    while (UART_READ(UART_SR_OFFSET) & UART_SR_TXFULL) {
        asm volatile("nop");
    }

    /* Write byte to FIFO */
    UART_WRITE(UART_FIFO_OFFSET, c);
    
    /* CRLF Conversion: If \n, send \r too */
    if (c == '\n') {
        while (UART_READ(UART_SR_OFFSET) & UART_SR_TXFULL);
        UART_WRITE(UART_FIFO_OFFSET, '\r');
    }

    console_uart.tx_count++;
}

void uart_send_string(const char *s) {
    while (*s) {
        uart_send_byte((uint8_t)*s++);
    }
}

uint8_t uart_recv_byte(void) {
    /* Wait until data is available (RXEMPTY must be 0) */
    while (UART_READ(UART_SR_OFFSET) & UART_SR_RXEMPTY) {
        asm volatile("nop");
    }

    console_uart.rx_count++;
    return (uint8_t)(UART_READ(UART_FIFO_OFFSET));
}

void uart_flush(void) {
    /* Wait until all bits are shifted out */
    while (!(UART_READ(UART_SR_OFFSET) & UART_SR_TXEMPTY));
}
