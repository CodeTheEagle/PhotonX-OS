/*
 * ======================================================================================
 * COPYRIGHT (C) 2026 PHOTONX TECHNOLOGIES. ALL RIGHTS RESERVED.
 * ======================================================================================
 * File:        gic_v2.h
 * Module:      GICv2 (Generic Interrupt Controller) Driver Interface
 * Platform:    Xilinx Zynq UltraScale+ MPSoC (ARM Cortex-A53)
 * Author:      PhotonX R&D Team
 * Date:        February 14, 2026
 *
 * DESCRIPTION:
 * This header file defines the register map, bitmasks, and data structures
 * for the ARM CoreLink GIC-400 Generic Interrupt Controller.
 *
 * The GIC is responsible for:
 * 1. Routing interrupts from peripherals (PL/PS) to specific CPU cores.
 * 2. Masking/Unmasking interrupts based on priority levels.
 * 3. Handling Inter-Processor Interrupts (IPI) for Multicore SMP.
 *
 * REFERENCE:
 * ARM Generic Interrupt Controller Architecture Specification (IHI 0048B)
 * ======================================================================================
 */

#ifndef _PHOTONX_DRIVERS_GIC_V2_H_
#define _PHOTONX_DRIVERS_GIC_V2_H_

#include <stdint.h>
#include "platform/zynqmp_hardware.h" // Ensures base addresses are correct

/* =========================================================================
 * GIC MEMORY MAP (DISTRIBUTOR & CPU INTERFACE)
 * =========================================================================
 * Base addresses are derived from the ZynqMP Hardware Map.
 * GIC_DIST_BASE: 0xF9010000
 * GIC_CPU_BASE:  0xF9020000
 */

#define GIC_DIST_BASE       0xF9010000UL
#define GIC_CPU_BASE        0xF9020000UL

/* =========================================================================
 * DISTRIBUTOR REGISTERS (GICD_)
 * Controls global interrupt routing and configuration.
 * ========================================================================= */
#define GICD_CTLR           (GIC_DIST_BASE + 0x000) // Distributor Control Register
#define GICD_TYPER          (GIC_DIST_BASE + 0x004) // Interrupt Controller Type
#define GICD_IIDR           (GIC_DIST_BASE + 0x008) // Implementer Identification

/* Interrupt Group Registers (Security State) */
#define GICD_IGROUPR(n)     (GIC_DIST_BASE + 0x080 + ((n) * 4))

/* Interrupt Set-Enable Registers (Enable IRQs) */
#define GICD_ISENABLER(n)   (GIC_DIST_BASE + 0x100 + ((n) * 4))

/* Interrupt Clear-Enable Registers (Disable IRQs) */
#define GICD_ICENABLER(n)   (GIC_DIST_BASE + 0x180 + ((n) * 4))

/* Interrupt Set-Pending Registers (Force IRQ) */
#define GICD_ISPENDR(n)     (GIC_DIST_BASE + 0x200 + ((n) * 4))

/* Interrupt Clear-Pending Registers (Clear State) */
#define GICD_ICPENDR(n)     (GIC_DIST_BASE + 0x280 + ((n) * 4))

/* Interrupt Active Bit Registers */
#define GICD_ISACTIVER(n)   (GIC_DIST_BASE + 0x300 + ((n) * 4))

/* Interrupt Priority Registers (0=Highest, 255=Lowest) */
#define GICD_IPRIORITYR(n)  (GIC_DIST_BASE + 0x400 + ((n) * 4))

/* Interrupt Processor Targets Registers (Route to Core 0-3) */
#define GICD_ITARGETSR(n)   (GIC_DIST_BASE + 0x800 + ((n) * 4))

/* Interrupt Configuration Registers (Edge vs Level Trigger) */
#define GICD_ICFGR(n)       (GIC_DIST_BASE + 0xC00 + ((n) * 4))

/* Software Generated Interrupt (SGI) Register */
#define GICD_SGIR           (GIC_DIST_BASE + 0xF00)

/* =========================================================================
 * CPU INTERFACE REGISTERS (GICC_)
 * Controls the interface between the GIC and the specific CPU Core.
 * ========================================================================= */
#define GICC_CTLR           (GIC_CPU_BASE + 0x0000) // CPU Interface Control
#define GICC_PMR            (GIC_CPU_BASE + 0x0004) // Priority Mask Register
#define GICC_BPR            (GIC_CPU_BASE + 0x0008) // Binary Point Register
#define GICC_IAR            (GIC_CPU_BASE + 0x000C) // Interrupt Acknowledge Register
#define GICC_EOIR           (GIC_CPU_BASE + 0x0010) // End of Interrupt Register
#define GICC_RPR            (GIC_CPU_BASE + 0x0014) // Running Priority Register
#define GICC_HPPIR          (GIC_CPU_BASE + 0x0018) // Highest Pending Interrupt
#define GICC_ABPR           (GIC_CPU_BASE + 0x001C) // Aliased Binary Point
#define GICC_IIDR           (GIC_CPU_BASE + 0x00FC) // CPU Interface Identification

/* =========================================================================
 * CONSTANTS & MASKS
 * ========================================================================= */
#define GICD_CTLR_ENABLE    0x1     // Enable Distributor
#define GICC_CTLR_ENABLE    0x1     // Enable CPU Interface

#define MAX_IRQS            1024    // Maximum supported interrupts in GICv2
#define IRQ_SGI_START       0       // Software Generated (0-15)
#define IRQ_PPI_START       16      // Private Peripheral (16-31)
#define IRQ_SPI_START       32      // Shared Peripheral (32-1019)

/* Priority Levels */
#define GIC_PRIO_HIGHEST    0x00
#define GIC_PRIO_HIGH       0x40
#define GIC_PRIO_MEDIUM     0x80    // Default
#define GIC_PRIO_LOW        0xC0
#define GIC_PRIO_LOWEST     0xF0

/* Target CPUs (One-hot encoding) */
#define TARGET_CPU0         (1 << 0)
#define TARGET_CPU1         (1 << 1)
#define TARGET_CPU2         (1 << 2)
#define TARGET_CPU3         (1 << 3)

/* =========================================================================
 * FUNCTION PROTOTYPES
 * ========================================================================= */
void gic_init(void);
void gic_enable_irq(uint32_t irq_id);
void gic_disable_irq(uint32_t irq_id);
void gic_set_priority(uint32_t irq_id, uint8_t priority);
void gic_set_target(uint32_t irq_id, uint8_t cpu_mask);
uint32_t gic_acknowledge_irq(void);
void gic_end_of_irq(uint32_t irq_id);

#endif /* _PHOTONX_DRIVERS_GIC_V2_H_ */
