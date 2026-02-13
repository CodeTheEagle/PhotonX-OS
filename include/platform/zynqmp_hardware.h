/*
 * Copyright (C) 2026 PhotonX Technologies.
 * All rights reserved.
 *
 * File: zynqmp_hardware.h
 * System: PhotonX-OS (HOCS Microkernel)
 * Target: Xilinx Zynq UltraScale+ MPSoC (ZU9EG / KV260)
 * Author: PhotonX R&D Team (Y. Cobanoglu)
 *
 * Description:
 * COMPREHENSIVE HARDWARE REGISTER MAP.
 * This file defines the physical base addresses, register offsets, and bitmasks
 * for the entire Zynq UltraScale+ Processing System (PS) and Programmable Logic (PL).
 *
 * Reference Documents:
 * - Xilinx UG1085: Zynq UltraScale+ Device TRM
 * - Xilinx UG1087: Register Reference
 *
 * WARNING: Direct access to these registers requires EL3/EL1 privileges.
 */

#ifndef _PHOTONX_ZYNQMP_HARDWARE_H_
#define _PHOTONX_ZYNQMP_HARDWARE_H_

#include <stdint.h>

/* =========================================================================
 * SECTION 1: GLOBAL SYSTEM MEMORY MAP (ARMv8 Physical Address Space)
 * ========================================================================= */

/* DDR4 SDRAM - Processing System */
#define ZYNQMP_DDR_LOW_BASE        0x00000000UL
#define ZYNQMP_DDR_LOW_SIZE        0x80000000UL  // Lower 2GB
#define ZYNQMP_DDR_HIGH_BASE       0x800000000UL // High Memory (starts at 32GB)
#define ZYNQMP_DDR_HIGH_SIZE       0x800000000UL // Up to 32GB Expansion

/* On-Chip Memory (OCM) - 256KB High-Speed SRAM */
#define ZYNQMP_OCM_BASE            0xFFFC0000UL
#define ZYNQMP_OCM_SIZE            0x00040000UL

/* QSPI Boot ROM (Linear Mode) */
#define ZYNQMP_QSPI_LINEAR_BASE    0xC0000000UL
#define ZYNQMP_QSPI_SIZE           0x20000000UL

/* PCIe (PCI Express) Address Space */
#define ZYNQMP_PCIE_LOW_BASE       0xE0000000UL
#define ZYNQMP_PCIE_HIGH_BASE      0x600000000UL
#define ZYNQMP_PCIE_IO_BASE        0xE0000000UL

/* Programmable Logic (FPGA) - HOCS Accelerator Interface */
#define ZYNQMP_PL_HPC0_BASE        0xA0000000UL  // AXI High-Performance Port 0
#define ZYNQMP_PL_HPC1_BASE        0xB0000000UL  // AXI High-Performance Port 1
#define ZYNQMP_PL_LPD_BASE         0x80000000UL  // AXI Low-Power Port

/* =========================================================================
 * SECTION 2: LOW POWER DOMAIN (LPD) BASE ADDRESSES
 * ========================================================================= */
#define ZYNQMP_UART0_BASE          0xFF000000UL  // Console / Debug
#define ZYNQMP_UART1_BASE          0xFF010000UL  // Aux Serial
#define ZYNQMP_I2C0_BASE           0xFF020000UL  // Sensor Bus 0
#define ZYNQMP_I2C1_BASE           0xFF030000UL  // Sensor Bus 1
#define ZYNQMP_SPI0_BASE           0xFF040000UL  // Flash Controller
#define ZYNQMP_SPI1_BASE           0xFF050000UL
#define ZYNQMP_CAN0_BASE           0xFF060000UL  // Automotive Bus
#define ZYNQMP_CAN1_BASE           0xFF070000UL
#define ZYNQMP_GPIO_BASE           0xFF0A0000UL  // General Purpose I/O
#define ZYNQMP_GEM0_BASE           0xFF0B0000UL  // Gigabit Ethernet 0
#define ZYNQMP_GEM1_BASE           0xFF0C0000UL  // Gigabit Ethernet 1
#define ZYNQMP_GEM2_BASE           0xFF0D0000UL  // Gigabit Ethernet 2
#define ZYNQMP_GEM3_BASE           0xFF0E0000UL  // Gigabit Ethernet 3
#define ZYNQMP_USB0_BASE           0xFF9D0000UL  // USB 3.0 Controller 0
#define ZYNQMP_USB1_BASE           0xFF9E0000UL  // USB 3.0 Controller 1
#define ZYNQMP_TTC0_BASE           0xFF110000UL  // Triple Timer Counter 0
#define ZYNQMP_TTC1_BASE           0xFF120000UL  // Triple Timer Counter 1
#define ZYNQMP_TTC2_BASE           0xFF130000UL  // Triple Timer Counter 2
#define ZYNQMP_TTC3_BASE           0xFF140000UL  // Triple Timer Counter 3

/* System Control Bases */
#define ZYNQMP_CRL_APB_BASE        0xFF5E0000UL  // Clock Reset LPD
#define ZYNQMP_IOU_SLCR_BASE       0xFF180000UL  // I/O Unit System Level Control
#define ZYNQMP_CSU_BASE            0xFFCA0000UL  // Config & Security Unit
#define ZYNQMP_PMU_BASE            0xFFD80000UL  // Platform Management Unit
#define ZYNQMP_LPD_SLCR_BASE       0xFF410000UL  // LPD System Level Control
/* =========================================================================
 * SECTION 3: CLOCK CONTROL & RESET REGISTERS (CRL_APB)
 * ========================================================================= */
/* * This block controls the PLLs (Phase Locked Loops) for the entire LPD.
 * WARNING: Modifying PLLs during operation will cause system crash.
 */

/* PLL Control Registers */
#define CRL_APB_RPLL_CTRL          (ZYNQMP_CRL_APB_BASE + 0x030) // RPU PLL
#define CRL_APB_RPLL_CFG           (ZYNQMP_CRL_APB_BASE + 0x034)
#define CRL_APB_RPLL_FRAC_CFG      (ZYNQMP_CRL_APB_BASE + 0x038)
#define CRL_APB_IOPLL_CTRL         (ZYNQMP_CRL_APB_BASE + 0x020) // I/O PLL
#define CRL_APB_IOPLL_CFG          (ZYNQMP_CRL_APB_BASE + 0x024)
#define CRL_APB_IOPLL_FRAC_CFG     (ZYNQMP_CRL_APB_BASE + 0x028)

/* Peripheral Reference Clocks (Divisors & Muxes) */
#define CRL_APB_USB3_DUAL_REF_CTRL (ZYNQMP_CRL_APB_BASE + 0x04C)
#define CRL_APB_GEM0_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x050)
#define CRL_APB_GEM1_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x054)
#define CRL_APB_GEM2_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x058)
#define CRL_APB_GEM3_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x05C)
#define CRL_APB_USB0_BUS_REF_CTRL  (ZYNQMP_CRL_APB_BASE + 0x060)
#define CRL_APB_USB1_BUS_REF_CTRL  (ZYNQMP_CRL_APB_BASE + 0x064)
#define CRL_APB_QSPI_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x068)
#define CRL_APB_SDIO0_REF_CTRL     (ZYNQMP_CRL_APB_BASE + 0x06C)
#define CRL_APB_SDIO1_REF_CTRL     (ZYNQMP_CRL_APB_BASE + 0x070)
#define CRL_APB_UART0_REF_CTRL     (ZYNQMP_CRL_APB_BASE + 0x074)
#define CRL_APB_UART1_REF_CTRL     (ZYNQMP_CRL_APB_BASE + 0x078)
#define CRL_APB_SPI0_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x07C)
#define CRL_APB_SPI1_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x080)
#define CRL_APB_CAN0_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x084)
#define CRL_APB_CAN1_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x088)
#define CRL_APB_CPU_R5_CTRL        (ZYNQMP_CRL_APB_BASE + 0x090)
#define CRL_APB_IOU_SWITCH_CTRL    (ZYNQMP_CRL_APB_BASE + 0x09C)
#define CRL_APB_CSU_PLL_CTRL       (ZYNQMP_CRL_APB_BASE + 0x0A0)
#define CRL_APB_PCAP_CTRL          (ZYNQMP_CRL_APB_BASE + 0x0A4)
#define CRL_APB_LPD_SWITCH_CTRL    (ZYNQMP_CRL_APB_BASE + 0x0A8)
#define CRL_APB_LPD_LSBUS_CTRL     (ZYNQMP_CRL_APB_BASE + 0x0AC)
#define CRL_APB_DBG_LPD_CTRL       (ZYNQMP_CRL_APB_BASE + 0x0B0)
#define CRL_APB_NAND_REF_CTRL      (ZYNQMP_CRL_APB_BASE + 0x0B4)
#define CRL_APB_LPD_DMA_REF_CTRL   (ZYNQMP_CRL_APB_BASE + 0x0B8)
#define CRL_APB_PL0_REF_CTRL       (ZYNQMP_CRL_APB_BASE + 0x0C0) // FPGA Clock 0
#define CRL_APB_PL1_REF_CTRL       (ZYNQMP_CRL_APB_BASE + 0x0C4) // FPGA Clock 1
#define CRL_APB_PL2_REF_CTRL       (ZYNQMP_CRL_APB_BASE + 0x0C8) // FPGA Clock 2
#define CRL_APB_PL3_REF_CTRL       (ZYNQMP_CRL_APB_BASE + 0x0CC) // FPGA Clock 3

/* LPD Reset Controls (Assert/Deassert) */
#define CRL_APB_RST_LPD_IOU0       (ZYNQMP_CRL_APB_BASE + 0x230)
#define CRL_APB_RST_LPD_IOU1       (ZYNQMP_CRL_APB_BASE + 0x234)
#define CRL_APB_RST_LPD_IOU2       (ZYNQMP_CRL_APB_BASE + 0x238)
#define CRL_APB_RST_LPD_TOP        (ZYNQMP_CRL_APB_BASE + 0x23C)
#define CRL_APB_RST_LPD_DBG        (ZYNQMP_CRL_APB_BASE + 0x240)

/* =========================================================================
 * SECTION 4: FULL POWER DOMAIN (FPD) & HIGH SPEED
 * ========================================================================= */
#define ZYNQMP_CRF_APB_BASE        0xFD1A0000UL  // Clock Reset FPD
#define ZYNQMP_SATA_BASE           0xFD0C0000UL  // SATA AHCI
#define ZYNQMP_PCIE_DMA_BASE       0xFD0F0000UL  // PCIe DMA
#define ZYNQMP_DP_BASE             0xFD4A0000UL  // DisplayPort
#define ZYNQMP_GPU_BASE            0xFD4B0000UL  // Mali-400 GPU
#define ZYNQMP_DDRC_BASE           0xFD070000UL  // DDR Controller

/* FPD PLL Controls */
#define CRF_APB_APLL_CTRL          (ZYNQMP_CRF_APB_BASE + 0x020) // ARM Core PLL
#define CRF_APB_APLL_CFG           (ZYNQMP_CRF_APB_BASE + 0x024)
#define CRF_APB_DPLL_CTRL          (ZYNQMP_CRF_APB_BASE + 0x02C) // DDR PLL
#define CRF_APB_DPLL_CFG           (ZYNQMP_CRF_APB_BASE + 0x030)
#define CRF_APB_VPLL_CTRL          (ZYNQMP_CRF_APB_BASE + 0x038) // Video PLL
#define CRF_APB_VPLL_CFG           (ZYNQMP_CRF_APB_BASE + 0x03C)

/* FPD Peripheral Clocks */
#define CRF_APB_ACPU_CTRL          (ZYNQMP_CRF_APB_BASE + 0x060) // A53 Clock
#define CRF_APB_DBG_TRACE_CTRL     (ZYNQMP_CRF_APB_BASE + 0x064)
#define CRF_APB_DBG_FPD_CTRL       (ZYNQMP_CRF_APB_BASE + 0x068)
#define CRF_APB_DP_VIDEO_REF_CTRL  (ZYNQMP_CRF_APB_BASE + 0x070)
#define CRF_APB_DP_AUDIO_REF_CTRL  (ZYNQMP_CRF_APB_BASE + 0x074)
#define CRF_APB_DP_STC_REF_CTRL    (ZYNQMP_CRF_APB_BASE + 0x078)
#define CRF_APB_DDR_CTRL           (ZYNQMP_CRF_APB_BASE + 0x080)
#define CRF_APB_GPU_REF_CTRL       (ZYNQMP_CRF_APB_BASE + 0x084)
#define CRF_APB_SATA_REF_CTRL      (ZYNQMP_CRF_APB_BASE + 0x0A0)
#define CRF_APB_PCIE_REF_CTRL      (ZYNQMP_CRF_APB_BASE + 0x0B4)
#define CRF_APB_GDMA_REF_CTRL      (ZYNQMP_CRF_APB_BASE + 0x0B8)
#define CRF_APB_DPDMA_REF_CTRL     (ZYNQMP_CRF_APB_BASE + 0x0BC)
/* =========================================================================
 * SECTION 5: HOCS (HYBRID OPTICAL COMPUTING SYSTEM) CUSTOM IP MAP
 * ========================================================================= */
/* * This section defines the register map for the PhotonX Optical Accelerator
 * residing in the Programmable Logic (PL). Access requires AXI4-Lite Bridge.
 * * Base Address: 0xA0000000 (Defined in Vivado Address Editor)
 */

#define HOCS_AXI_BASE              0xA0000000UL
#define HOCS_AXI_RANGE             0x00010000UL  // 64KB Register Space

/* Optical Control Unit (OCU) Registers */
#define HOCS_REG_CONTROL           (HOCS_AXI_BASE + 0x0000) // Control Register
#define HOCS_REG_STATUS            (HOCS_AXI_BASE + 0x0004) // Status Register
#define HOCS_REG_IRQ_ENABLE        (HOCS_AXI_BASE + 0x0008) // Interrupt Enable
#define HOCS_REG_IRQ_STATUS        (HOCS_AXI_BASE + 0x000C) // Interrupt Status

/* Optical Matrix Configuration */
#define HOCS_REG_MATRIX_DIM        (HOCS_AXI_BASE + 0x0010) // Dimension N (NxN)
#define HOCS_REG_WAVELENGTH        (HOCS_AXI_BASE + 0x0014) // Laser Wavelength (nm)
#define HOCS_REG_PHASE_SHIFT       (HOCS_AXI_BASE + 0x0018) // Phase Modulator
#define HOCS_REG_LASER_POWER       (HOCS_AXI_BASE + 0x001C) // Global Laser Power

/* DMA Pointers (Direct Memory Access) */
#define HOCS_REG_SRC_ADDR_L        (HOCS_AXI_BASE + 0x0020) // Source Ptr Low
#define HOCS_REG_SRC_ADDR_H        (HOCS_AXI_BASE + 0x0024) // Source Ptr High
#define HOCS_REG_DST_ADDR_L        (HOCS_AXI_BASE + 0x0028) // Dest Ptr Low
#define HOCS_REG_DST_ADDR_H        (HOCS_AXI_BASE + 0x002C) // Dest Ptr High

/* Thermal Management (VCSEL Array) */
#define HOCS_REG_TEMP_SENSOR_1     (HOCS_AXI_BASE + 0x0040) // Zone 1 Temp
#define HOCS_REG_TEMP_SENSOR_2     (HOCS_AXI_BASE + 0x0044) // Zone 2 Temp
#define HOCS_REG_TEC_CONTROL       (HOCS_AXI_BASE + 0x0048) // Peltier Control

/* Control Bitmasks */
#define HOCS_CTRL_START            (1 << 0)  // Start Computation
#define HOCS_CTRL_RESET            (1 << 1)  // Soft Reset IP
#define HOCS_CTRL_DMA_EN           (1 << 2)  // Enable DMA Engine
#define HOCS_CTRL_LASER_EN         (1 << 3)  // Activate Lasers

/* Status Bitmasks */
#define HOCS_STATUS_IDLE           (1 << 0)
#define HOCS_STATUS_BUSY           (1 << 1)
#define HOCS_STATUS_DONE           (1 << 2)
#define HOCS_STATUS_ERROR          (1 << 3)
#define HOCS_STATUS_OVERHEAT       (1 << 4)

/* * struct hocs_device
 * C-Structure for easy driver access
 */
typedef struct {
    volatile uint32_t control;        // 0x00
    volatile uint32_t status;         // 0x04
    volatile uint32_t irq_enable;     // 0x08
    volatile uint32_t irq_status;     // 0x0C
    volatile uint32_t matrix_dim;     // 0x10
    volatile uint32_t wavelength;     // 0x14
    volatile uint32_t phase_shift;    // 0x18
    volatile uint32_t laser_power;    // 0x1C
    volatile uint32_t src_addr_l;     // 0x20
    volatile uint32_t src_addr_h;     // 0x24
    volatile uint32_t dst_addr_l;     // 0x28
    volatile uint32_t dst_addr_h;     // 0x2C
    volatile uint32_t reserved[4];    // 0x30-0x3C (Padding)
    volatile uint32_t temp_sensor[4]; // 0x40-0x4C
} hocs_hw_t;

#endif /* _PHOTONX_ZYNQMP_HARDWARE_H_ */
