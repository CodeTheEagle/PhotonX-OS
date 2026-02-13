/*
 * Copyright (C) 2026 PhotonX Technologies. All rights reserved.
 *
 * PROPRIETARY AND CONFIDENTIAL.
 *
 * File: zynqmp_hardware.h
 * Author: PhotonX System Architect (Y. Cobanoglu)
 * Date: 2026-02-13
 * Target: Xilinx Zynq UltraScale+ MPSoC (ARMv8-A)
 *
 * Description:
 * This file contains the comprehensive physical address mapping,
 * register definitions, and bit-field macros for the HOCS (PhotonX)
 * Microkernel. It covers the Processing System (PS), Programmable Logic (PL)
 * interfaces, High-Speed Bus (AXI), and Optical Control Units.
 *
 * WARNING: Direct access to reserved memory regions may cause
 * system instability or bus hang. Verify all addresses against
 * Xilinx UG1085 (Zynq UltraScale+ TRM).
 */

#ifndef _PHOTONX_ZYNQMP_HARDWARE_H_
#define _PHOTONX_ZYNQMP_HARDWARE_H_

#include <stdint.h>

/* =========================================================================
 * SECTION 1: MEMORY MAP (PHYSICAL)
 * ========================================================================= */

/*
 * DDR4 SDRAM Layout
 * 0x0000_0000 to 0x7FFF_FFFF (Lower 2GB)
 * 0x8_0000_0000 to 0xF_FFFF_FFFF (Upper 32GB - If equipped)
 */
#define PHYS_SDRAM_BASE            0x00000000UL
#define PHYS_SDRAM_SIZE            0x80000000UL  // 2GB Default

/*
 * Low Power Domain (LPD) Peripherals
 */
#define ZYNQMP_UART0_BASE          0xFF000000UL
#define ZYNQMP_UART1_BASE          0xFF010000UL
#define ZYNQMP_I2C0_BASE           0xFF020000UL
#define ZYNQMP_I2C1_BASE           0xFF030000UL
#define ZYNQMP_SPI0_BASE           0xFF040000UL
#define ZYNQMP_SPI1_BASE           0xFF050000UL
#define ZYNQMP_CAN0_BASE           0xFF060000UL
#define ZYNQMP_CAN1_BASE           0xFF070000UL
#define ZYNQMP_GPIO_BASE           0xFF0A0000UL
#define ZYNQMP_GEM0_BASE           0xFF0B0000UL  // Gigabit Ethernet 0
#define ZYNQMP_GEM1_BASE           0xFF0C0000UL  // Gigabit Ethernet 1
#define ZYNQMP_GEM2_BASE           0xFF0D0000UL  // Gigabit Ethernet 2
#define ZYNQMP_GEM3_BASE           0xFF0E0000UL  // Gigabit Ethernet 3
#define ZYNQMP_USB0_BASE           0xFF9D0000UL

/*
 * Full Power Domain (FPD) Peripherals
 */
#define ZYNQMP_SATA_BASE           0xFD0C0000UL
#define ZYNQMP_PCIE_BASE           0xFD0E0000UL
#define ZYNQMP_DP_BASE             0xFD4A0000UL  // DisplayPort
#define ZYNQMP_GPU_BASE            0xFD4B0000UL  // Mali-400 MP2

/*
 * System Control & Power Management
 */
#define ZYNQMP_CRL_APB_BASE        0xFF5E0000UL  // Clock Control (LPD)
#define ZYNQMP_CRF_APB_BASE        0xFD1A0000UL  // Clock Control (FPD)
#define ZYNQMP_PMU_GLOBAL_BASE     0xFFD80000UL
#define ZYNQMP_CSU_BASE            0xFFCA0000UL

/* =========================================================================
 * SECTION 2: INTERRUPT CONTROLLER (GIC-400/500)
 * ========================================================================= */
#define GIC_DIST_BASE              0xF9010000UL  // Distributor
#define GIC_CPU_BASE               0xF9020000UL  // CPU Interface

/* Distributor Registers */
#define GICD_CTLR                  (GIC_DIST_BASE + 0x000)
#define GICD_TYPER                 (GIC_DIST_BASE + 0x004)
#define GICD_IIDR                  (GIC_DIST_BASE + 0x008)
#define GICD_IGROUPR(n)            (GIC_DIST_BASE + 0x080 + (n) * 4)
#define GICD_ISENABLER(n)          (GIC_DIST_BASE + 0x100 + (n) * 4)
#define GICD_ICENABLER(n)          (GIC_DIST_BASE + 0x180 + (n) * 4)
#define GICD_ISPENDR(n)            (GIC_DIST_BASE + 0x200 + (n) * 4)
#define GICD_ICPENDR(n)            (GIC_DIST_BASE + 0x280 + (n) * 4)
#define GICD_IPRIORITYR(n)         (GIC_DIST_BASE + 0x400 + (n) * 4)
#define GICD_ITARGETSR(n)          (GIC_DIST_BASE + 0x800 + (n) * 4)
#define GICD_ICFGR(n)              (GIC_DIST_BASE + 0xC00 + (n) * 4)

/* CPU Interface Registers */
#define GICC_CTLR                  (GIC_CPU_BASE + 0x0000)
#define GICC_PMR                   (GIC_CPU_BASE + 0x0004)
#define GICC_BPR                   (GIC_CPU_BASE + 0x0008)
#define GICC_IAR                   (GIC_CPU_BASE + 0x000C)
#define GICC_EOIR                  (GIC_CPU_BASE + 0x0010)
#define GICC_RPR                   (GIC_CPU_BASE + 0x0014)
#define GICC_HPPIR                 (GIC_CPU_BASE + 0x0018)

/* =========================================================================
 * SECTION 3: UART CONTROL REGISTERS (Cadence)
 * ========================================================================= */
typedef struct {
    volatile uint32_t Control;          // 0x00 - Channel Control
    volatile uint32_t Mode;             // 0x04 - Channel Mode
    volatile uint32_t Intr_En;          // 0x08 - Interrupt Enable
    volatile uint32_t Intr_Dis;         // 0x0C - Interrupt Disable
    volatile uint32_t Intr_Mask;        // 0x10 - Interrupt Mask
    volatile uint32_t Chnl_Intr_Sts;    // 0x14 - Interrupt Status
    volatile uint32_t Baud_Rate_Gen;    // 0x18 - Baud Rate Generator
    volatile uint32_t Rx_Timeout;       // 0x1C - Receiver Timeout
    volatile uint32_t Rx_FIFO_Trig;     // 0x20 - Receiver FIFO Trigger
    volatile uint32_t Modem_Ctrl;       // 0x24 - Modem Control
    volatile uint32_t Modem_Sts;        // 0x28 - Modem Status
    volatile uint32_t Channel_Sts;      // 0x2C - Channel Status
    volatile uint32_t TX_RX_FIFO;       // 0x30 - Transmit/Receive FIFO
    volatile uint32_t Baud_Rate_Div;    // 0x34 - Baud Rate Divider
    volatile uint32_t Flow_Delay;       // 0x38 - Flow Control Delay
    volatile uint32_t Tx_FIFO_Trig;     // 0x44 - Transmitter FIFO Trigger
} uart_regs_t;

/* Bit Definitions for UART Control */
#define UART_CR_STOPBRK             0x00000100  // Stop Break
#define UART_CR_STARTBRK            0x00000080  // Start Break
#define UART_CR_TORST               0x00000040  // Timeout Reset
#define UART_CR_TX_DIS              0x00000020  // TX Disable
#define UART_CR_TX_EN               0x00000010  // TX Enable
#define UART_CR_RX_DIS              0x00000008  // RX Disable
#define UART_CR_RX_EN               0x00000004  // RX Enable
#define UART_CR_TXRST               0x00000002  // TX Logic Reset
#define UART_CR_RXRST               0x00000001  // RX Logic Reset

/* =========================================================================
 * SECTION 4: HOCS OPTICAL INTERFACE (PL - FPGA MAPPING)
 * ========================================================================= */
/*
 * This section maps the AXI4-Lite bus addresses where the Custom HOCS IP
 * resides in the FPGA fabric. These addresses MUST match the Vivado
 * Block Design Address Editor.
 */

#define HOCS_AXI_BASE              0xA0000000UL  // High-Performance Port 0
#define HOCS_AXI_RANGE             0x00100000UL  // 1MB Address Space

/* Optical Control Registers (Relative to HOCS_AXI_BASE) */
#define HOCS_REG_CONTROL           0x0000        // Master Control (Start/Stop)
#define HOCS_REG_STATUS            0x0004        // Status (Busy/Done/Error)
#define HOCS_REG_CONFIG            0x0008        // Optical Wavelength Config
#define HOCS_REG_INT_MASK          0x000C        // Interrupt Mask
#define HOCS_REG_MATRIX_DIM        0x0010        // Matrix Dimension (N x N)
#define HOCS_REG_DMA_SRC           0x0014        // Source Address (DDR)
#define HOCS_REG_DMA_DST           0x0018        // Destination Address (DDR)
#define HOCS_REG_THERMAL           0x0020        // Laser Temperature Sensor
#define HOCS_REG_FIBONACCI_COEF    0x0024        // Spiral Algo Coef

/* HOCS Status Flags */
#define HOCS_STS_IDLE              (1 << 0)
#define HOCS_STS_BUSY              (1 << 1)
#define HOCS_STS_DMA_ERR           (1 << 2)
#define HOCS_STS_OPT_ALIGN_ERR     (1 << 3)      // Optical Misalignment
#define HOCS_STS_THERMAL_WARN      (1 << 4)

/* * [TODO]: Add definitions for 144-Channel VCSEL Array individual control
 * Requires expanding the register map from 0x1000 to 0x2000
 */

#endif /* _PHOTONX_ZYNQMP_HARDWARE_H_ */
