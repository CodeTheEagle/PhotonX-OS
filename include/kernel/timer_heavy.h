/*
 * ======================================================================================
 * COPYRIGHT (C) 2026 PHOTONX TECHNOLOGIES. ALL RIGHTS RESERVED.
 * ======================================================================================
 * File:        include/kernel/timer_heavy.h
 * Module:      High-Precision ARMv8 Generic Timer Driver (HAL)
 * Platform:    Xilinx Zynq UltraScale+ MPSoC (Cortex-A53 / R5)
 * Architecture: AArch64 (ARMv8-A)
 * Author:      PhotonX R&D Team (Lead Architect: Y. Cobanoglu)
 * Date:        February 14, 2026
 *
 * DESCRIPTION:
 * This header file provides the exhaustive register map and control structures
 * for the ARM Generic Timer System. It supports:
 * - EL1 Physical Timer (CNTP)
 * - EL1 Virtual Timer (CNTV)
 * - EL2 Hypervisor Timer (CNTHP)
 * - EL3 Secure Timer (CNTPS)
 *
 * It enables nanosecond-level precision for Optical Computing synchronization
 * and guarantees deterministic latency for Real-Time Scheduling.
 *
 * REFERENCE:
 * ARM Architecture Reference Manual ARMv8, for ARMv8-A architecture profile
 * Section D6: The Generic Timer
 * ======================================================================================
 */

#ifndef _PHOTONX_KERNEL_TIMER_HEAVY_H_
#define _PHOTONX_KERNEL_TIMER_HEAVY_H_

#include <stdint.h>
#include "platform/zynqmp_hardware.h"

/* =========================================================================
 * SECTION 1: SYSTEM REGISTER DEFINITIONS (AArch64)
 * =========================================================================
 * These registers are accessed via MRS/MSR instructions.
 * Memory-mapped access is also available but slower.
 */

/* 1.1 Physical Timer Registers (EL1) */
#define REG_CNTP_TVAL_EL0       "cntp_tval_el0"   // Timer Value (Down Counter)
#define REG_CNTP_CTL_EL0        "cntp_ctl_el0"    // Control Register
#define REG_CNTP_CVAL_EL0       "cntp_cval_el0"   // Compare Value (Absolute)
#define REG_CNTPCT_EL0          "cntpct_el0"      // Physical Count (64-bit)

/* 1.2 Virtual Timer Registers (EL1) - Used for Virtualization */
#define REG_CNTV_TVAL_EL0       "cntv_tval_el0"
#define REG_CNTV_CTL_EL0        "cntv_ctl_el0"
#define REG_CNTV_CVAL_EL0       "cntv_cval_el0"
#define REG_CNTVCT_EL0          "cntvct_el0"      // Virtual Count

/* 1.3 Hypervisor Timer Registers (EL2) */
#define REG_CNTHP_TVAL_EL2      "cnthp_tval_el2"
#define REG_CNTHP_CTL_EL2       "cnthp_ctl_el2"
#define REG_CNTHP_CVAL_EL2      "cnthp_cval_el2"

/* 1.4 Secure Timer Registers (EL3) */
#define REG_CNTPS_TVAL_EL1      "cntps_tval_el1"
#define REG_CNTPS_CTL_EL1       "cntps_ctl_el1"
#define REG_CNTPS_CVAL_EL1      "cntps_cval_el1"

/* 1.5 Counter Frequency Register */
#define REG_CNTFRQ_EL0          "cntfrq_el0"      // System Frequency

/* =========================================================================
 * SECTION 2: BITMASKS AND CONTROL FLAGS
 * =========================================================================
 */

/* Control Register (ENABLE bit) */
#define TIMER_ENABLE_BIT        (1UL << 0)        // 1 = Timer Enabled
#define TIMER_DISABLE_BIT       (0UL << 0)        // 0 = Timer Disabled

/* Control Register (IMASK bit) */
#define TIMER_IMASK_BIT         (1UL << 1)        // 1 = Interrupt Masked (No IRQ)
#define TIMER_UNMASK_BIT        (0UL << 1)        // 0 = Interrupt Unmasked (Fire IRQ)

/* Control Register (ISTATUS bit) */
#define TIMER_ISTATUS_BIT       (1UL << 2)        // 1 = Condition Met (IRQ Pending)

/* Counter Frequency Constants (Xilinx ZynqMP Specific) */
#define ZYNQMP_REF_CLK_HZ       100000000UL       // 100 MHz Default Reference
#define NS_PER_SEC              1000000000UL      // Nanoseconds per second
#define US_PER_SEC              1000000UL         // Microseconds per second

/* =========================================================================
 * SECTION 3: DATA STRUCTURES
 * =========================================================================
 */

/*
 * struct timer_config_t
 * Holds the configuration for a specific hardware timer instance.
 */
typedef struct {
    uint64_t frequency_hz;      // Detected frequency
    uint64_t min_delta_ticks;   // Minimum programming delta (safety margin)
    uint64_t max_delta_ticks;   // Maximum counter value before wrap
    uint32_t irq_number;        // GIC Interrupt ID (PPI)
    uint8_t  initialized;       // Initialization Flag
    uint8_t  use_virtual;       // 1 = Use Virtual Timer, 0 = Physical
} timer_config_t;

/*
 * struct system_uptime_t
 * High-precision uptime tracker using 64-bit accumulators.
 */
typedef struct {
    uint64_t boot_timestamp;    // Raw cycle count at boot
    uint64_t last_tick;         // Raw cycle count at last IRQ
    uint64_t uptime_ns;         // Total uptime in nanoseconds
    uint64_t uptime_sec;        // Total uptime in seconds
    uint64_t ticks_per_ns;      // Pre-calculated conversion factor
} system_uptime_t;

/* Global Accessors */
extern volatile system_uptime_t sys_uptime;
extern timer_config_t sys_timer_config;

/* Function Prototypes */
void timer_core_init(void);
void timer_calibrate_delay(void);
void timer_set_timeout(uint64_t ns);
void timer_enable_irq(void);
void timer_disable_irq(void);
uint64_t timer_get_timestamp_ns(void);
void udelay(uint64_t usecs);
void mdelay(uint64_t msecs);

#endif /* _PHOTONX_KERNEL_TIMER_HEAVY_H_ */
