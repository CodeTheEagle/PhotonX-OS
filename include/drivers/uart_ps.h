/*
 * ======================================================================================
 * COPYRIGHT (C) 2026 PHOTONX TECHNOLOGIES. ALL RIGHTS RESERVED.
 * ======================================================================================
 * File:        uart_ps.h
 * Module:      Cadence UART Driver Interface (Xilinx ZynqMP)
 * Author:      PhotonX R&D Team (System Software Group)
 * Platform:    ARMv8-A (Cortex-A53)
 *
 * DESCRIPTION:
 * This header defines the register map, bit definitions, and data structures
 * for the Cadence UART controller used in the Zynq UltraScale+ MPSoC.
 *
 * FEATURES:
 * - Full Duplex Communication
 * - Programmable Baud Rate Generator
 * - 64-byte Transmit and Receive FIFOs
 * - Loopback Mode for Self-Test
 * - Interrupt-driven I/O with Ring Buffers
 * ======================================================================================
 */

#ifndef _PHOTONX_DRIVERS_UART_PS_H_
#define _PHOTONX_DRIVERS_UART_PS_H_

#include <stdint.h>
#include "platform/zynqmp_hardware.h"

/* =========================================================================
 * UART REGISTER MAP (OFFSETS)
 * =========================================================================
 */
#define UART_CR_OFFSET          0x0000  /* Control Register */
#define UART_MR_OFFSET          0x0004  /* Mode Register */
#define UART_IER_OFFSET         0x0008  /* Interrupt Enable Register */
#define UART_IDR_OFFSET         0x000C  /* Interrupt Disable Register */
#define UART_IMR_OFFSET         0x0010  /* Interrupt Mask Register */
#define UART_ISR_OFFSET         0x0014  /* Interrupt Status Register */
#define UART_BAUDGEN_OFFSET     0x0018  /* Baud Rate Generator */
#define UART_RXTOUT_OFFSET      0x001C  /* Receiver Timeout */
#define UART_RXWM_OFFSET        0x0020  /* Receiver FIFO Trigger Level */
#define UART_MODEMCR_OFFSET     0x0024  /* Modem Control */
#define UART_MODEMSR_OFFSET     0x0028  /* Modem Status */
#define UART_SR_OFFSET          0x002C  /* Channel Status */
#define UART_FIFO_OFFSET        0x0030  /* Transmit/Receive FIFO */
#define UART_BAUDDIV_OFFSET     0x0034  /* Baud Rate Divider */
#define UART_FLOWDEL_OFFSET     0x0038  /* Flow Control Delay */
#define UART_TXWM_OFFSET        0x0044  /* Transmitter FIFO Trigger Level */

/* =========================================================================
 * BIT DEFINITIONS: CONTROL REGISTER (CR)
 * =========================================================================
 */
#define UART_CR_STOPBRK         0x00000100  /* Stop transmission of break */
#define UART_CR_STARTBRK        0x00000080  /* Start transmission of break */
#define UART_CR_TORST           0x00000040  /* Restart receiver timeout */
#define UART_CR_TX_DIS          0x00000020  /* Transmit Disable */
#define UART_CR_TX_EN           0x00000010  /* Transmit Enable */
#define UART_CR_RX_DIS          0x00000008  /* Receive Disable */
#define UART_CR_RX_EN           0x00000004  /* Receive Enable */
#define UART_CR_TXRST           0x00000002  /* Transmit Logic Reset */
#define UART_CR_RXRST           0x00000001  /* Receive Logic Reset */

/* =========================================================================
 * BIT DEFINITIONS: MODE REGISTER (MR)
 * =========================================================================
 */
#define UART_MR_CCLK            0x00000400  /* Input clock selection */
#define UART_MR_CHMODE_NORM     0x00000000  /* Normal Mode */
#define UART_MR_CHMODE_ECHO     0x00000100  /* Auto Echo */
#define UART_MR_CHMODE_L_LOOP   0x00000200  /* Local Loopback */
#define UART_MR_CHMODE_R_LOOP   0x00000300  /* Remote Loopback */

#define UART_MR_NBSTOP_1        0x00000000  /* 1 Stop Bit */
#define UART_MR_NBSTOP_1_5      0x00000040  /* 1.5 Stop Bits */
#define UART_MR_NBSTOP_2        0x00000080  /* 2 Stop Bits */

#define UART_MR_PAR_EVEN        0x00000000  /* Even Parity */
#define UART_MR_PAR_ODD         0x00000008  /* Odd Parity */
#define UART_MR_PAR_SPACE       0x00000010  /* Space Parity */
#define UART_MR_PAR_MARK        0x00000018  /* Mark Parity */
#define UART_MR_PAR_NONE        0x00000020  /* No Parity */

#define UART_MR_CHARLEN_6       0x00000006  /* 6 bits */
#define UART_MR_CHARLEN_7       0x00000004  /* 7 bits */
#define UART_MR_CHARLEN_8       0x00000000  /* 8 bits */

/* =========================================================================
 * BIT DEFINITIONS: CHANNEL STATUS REGISTER (SR)
 * =========================================================================
 */
#define UART_SR_TNFUL           0x00004000  /* TX FIFO Nearly Full */
#define UART_SR_TGTRIG          0x00002000  /* TX FIFO Trigger */
#define UART_SR_FLOWDEL         0x00001000  /* Flow Delay */
#define UART_SR_TACTIVE         0x00000800  /* TX Active */
#define UART_SR_RACTIVE         0x00000400  /* RX Active */
#define UART_SR_TXFULL          0x00000010  /* TX FIFO Full */
#define UART_SR_TXEMPTY         0x00000008  /* TX FIFO Empty */
#define UART_SR_RXFULL          0x00000004  /* RX FIFO Full */
#define UART_SR_RXEMPTY         0x00000002  /* RX FIFO Empty */
#define UART_SR_RGTRIG          0x00000001  /* RX FIFO Trigger */

/* =========================================================================
 * DATA STRUCTURES: RING BUFFER
 * =========================================================================
 */
#define UART_RING_BUFFER_SIZE   2048        /* 2KB Buffer for Console */

typedef struct {
    uint8_t  buffer[UART_RING_BUFFER_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
} ring_buffer_t;

typedef struct {
    uint32_t base_addr;
    uint32_t baud_rate;
    uint32_t irq_num;
    ring_buffer_t tx_buffer;
    ring_buffer_t rx_buffer;
    uint64_t tx_count;
    uint64_t rx_count;
    uint64_t error_count;
} uart_driver_t;

/* Global Driver Instance */
extern uart_driver_t console_uart;

/* Function Prototypes */
void uart_init_controller(void);
void uart_send_byte(uint8_t c);
uint8_t uart_recv_byte(void);
void uart_send_string(const char *s);
int uart_is_busy(void);
void uart_flush(void);
void uart_interrupt_handler(void);

#endif /* _PHOTONX_DRIVERS_UART_PS_H_ */
