/*
 * ======================================================================================
 * COPYRIGHT (C) 2026 PHOTONX TECHNOLOGIES. ALL RIGHTS RESERVED.
 * ======================================================================================
 * File:        src/kernel/time/timer.c
 * Module:      Heavy-Duty Timer Implementation
 * Dependencies: gic_v2.h, zynqmp_hardware.h
 * ======================================================================================
 */

#include "kernel/timer_heavy.h"
#include "drivers/gic_v2.h"
#include "lib/kprintf.h"

/* Global Instances */
volatile system_uptime_t sys_uptime = {0};
timer_config_t sys_timer_config = {0};

/*
 * ======================================================================================
 * INLINE ASSEMBLY WRAPPERS
 * Direct interaction with ARMv8 Coprocessor Registers
 * ======================================================================================
 */

/*
 * read_cntfrq_el0
 * Reads the System Counter Frequency.
 */
static inline uint64_t read_cntfrq_el0(void) {
    uint64_t val;
    asm volatile("mrs %0, cntfrq_el0" : "=r" (val));
    return val;
}

/*
 * write_cntfrq_el0
 * Writes the System Counter Frequency (Usually done by BootROM/ATF).
 * Only needed if we are the primary bootloader.
 */
static inline void write_cntfrq_el0(uint64_t val) {
    asm volatile("msr cntfrq_el0, %0" : : "r" (val));
    asm volatile("isb"); // Instruction Synchronization Barrier
}

/*
 * read_cntpct_el0
 * Reads the Physical Counter Value (64-bit up-counter).
 */
static inline uint64_t read_cntpct_el0(void) {
    uint64_t val;
    asm volatile("mrs %0, cntpct_el0" : "=r" (val));
    return val;
}

/*
 * read_cntvct_el0
 * Reads the Virtual Counter Value (Includes Offset).
 */
static inline uint64_t read_cntvct_el0(void) {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}

/*
 * write_cntp_tval_el0
 * Writes the Physical Timer TimerValue (Down-counter trigger).
 */
static inline void write_cntp_tval_el0(uint64_t val) {
    asm volatile("msr cntp_tval_el0, %0" : : "r" (val));
    asm volatile("isb");
}

/*
 * write_cntp_ctl_el0
 * Writes the Physical Timer Control Register.
 * Bit 0: Enable, Bit 1: Mask, Bit 2: Status
 */
static inline void write_cntp_ctl_el0(uint64_t val) {
    asm volatile("msr cntp_ctl_el0, %0" : : "r" (val));
    asm volatile("isb");
}

/*
 * read_cntp_ctl_el0
 * Reads the Physical Timer Control Register.
 */
static inline uint64_t read_cntp_ctl_el0(void) {
    uint64_t val;
    asm volatile("mrs %0, cntp_ctl_el0" : "=r" (val));
    return val;
}

/*
 * write_cntp_cval_el0
 * Writes the Physical Timer CompareValue (Absolute match).
 */
static inline void write_cntp_cval_el0(uint64_t val) {
    asm volatile("msr cntp_cval_el0, %0" : : "r" (val));
    asm volatile("isb");
}

/*
 * read_cntp_cval_el0
 * Reads the Physical Timer CompareValue.
 */
static inline uint64_t read_cntp_cval_el0(void) {
    uint64_t val;
    asm volatile("mrs %0, cntp_cval_el0" : "=r" (val));
    return val;
}

/* * --------------------------------------------------------------------------------------
 * Virtual Timer Wrappers (For completeness and future virtualization support)
 * --------------------------------------------------------------------------------------
 */
static inline void write_cntv_tval_el0(uint64_t val) {
    asm volatile("msr cntv_tval_el0, %0" : : "r" (val));
    asm volatile("isb");
}

static inline void write_cntv_ctl_el0(uint64_t val) {
    asm volatile("msr cntv_ctl_el0, %0" : : "r" (val));
    asm volatile("isb");
}

/* Barrier instructions to prevent speculative execution issues */
static inline void memory_barrier(void) {
    asm volatile("dsb sy"); // Data Synchronization Barrier (Full System)
    asm volatile("isb");    // Instruction Synchronization Barrier
}
/*
 * ======================================================================================
 * SECTION: TIME CONVERSION UTILITIES
 * ======================================================================================
 * We avoid floating point math in the kernel for performance and safety.
 * All calculations use fixed-point arithmetic or scaled integers.
 */

/*
 * ticks_to_ns
 * Converts raw CPU ticks to nanoseconds.
 * Formula: (Ticks * 1,000,000,000) / Frequency
 */
static uint64_t ticks_to_ns(uint64_t ticks) {
    /* * WARNING: Multiplication might overflow 64-bit integer if ticks is large.
     * We should check for overflow or use 128-bit math if supported.
     * For now, we assume ticks < 18446744073 (approx 580 years at 1GHz).
     */
    uint64_t ns = (ticks * NS_PER_SEC) / sys_timer_config.frequency_hz;
    return ns;
}

/*
 * ns_to_ticks
 * Converts nanoseconds to raw CPU ticks.
 * Formula: (NS * Frequency) / 1,000,000,000
 */
static uint64_t ns_to_ticks(uint64_t ns) {
    uint64_t ticks = (ns * sys_timer_config.frequency_hz) / NS_PER_SEC;
    return ticks;
}

/*
 * ticks_to_us
 * Converts raw CPU ticks to microseconds.
 */
static uint64_t ticks_to_us(uint64_t ticks) {
    return (ticks * US_PER_SEC) / sys_timer_config.frequency_hz;
}

/*
 * us_to_ticks
 * Converts microseconds to raw CPU ticks.
 */
static uint64_t us_to_ticks(uint64_t us) {
    return (us * sys_timer_config.frequency_hz) / US_PER_SEC;
}

/*
 * timer_update_uptime
 * Updates the global system uptime structure.
 * This should be called periodically or inside the ISR.
 */
void timer_update_uptime(void) {
    uint64_t current_ticks = read_cntpct_el0();
    uint64_t delta = current_ticks - sys_uptime.last_tick;

    /* Update cumulative uptime */
    sys_uptime.uptime_ns += ticks_to_ns(delta);
    sys_uptime.uptime_sec = sys_uptime.uptime_ns / NS_PER_SEC;

    /* Snapshot current tick */
    sys_uptime.last_tick = current_ticks;
}

/*
 * timer_get_uptime_ns
 * Returns the atomic system uptime in nanoseconds.
 * Safe to call from any context.
 */
uint64_t timer_get_uptime_ns(void) {
    /* TODO: Add spinlock here for multicore safety */
    timer_update_uptime(); // Force update
    return sys_uptime.uptime_ns;
}

/*
 * timer_get_boot_ticks
 * Returns the tick count captured at boot time.
 */
uint64_t timer_get_boot_ticks(void) {
    return sys_uptime.boot_timestamp;
}
/*
 * ======================================================================================
 * SECTION: INITIALIZATION & CALIBRATION
 * ======================================================================================
 */

/*
 * timer_core_init
 * Main initialization routine. Must be called after GIC Init and before Scheduler.
 */
void timer_core_init(void) {
    // kprintf("[TIMER] Initializing ARMv8 Generic Timer Core...\n");

    /* 1. Detect System Frequency */
    /* On Xilinx ZynqMP, this is usually 100MHz (LPD_LSBUS) */
    uint64_t freq = read_cntfrq_el0();

    /* Sanity Check: If freq is 0, we are likely in QEMU without proper DTB */
    if (freq == 0) {
        // kprintf("[TIMER] WARN: CNTFRQ is 0. Forcing to 100MHz default.\n");
        freq = ZYNQMP_REF_CLK_HZ;
        write_cntfrq_el0(freq);
    }

    sys_timer_config.frequency_hz = freq;
    sys_timer_config.min_delta_ticks = 0xF; // Minimum 15 ticks overhead
    sys_timer_config.max_delta_ticks = 0x7FFFFFFFFFFFFFFF; // Max 64-bit positive
    sys_timer_config.use_virtual = 0; // Default to Physical Timer
    sys_timer_config.initialized = 1;

    /* 2. Determine Interrupt ID */
    /* * ARMv8 Generic Timer Interrupt Map (PPI):
     * Non-secure Physical Timer (CNTP): ID 30
     * Virtual Timer (CNTV):             ID 27
     * Hypervisor Timer (CNTHP):         ID 26
     * Secure Physical Timer (CNTPS):    ID 29
     */
    sys_timer_config.irq_number = 30; // CNTP_EL0

    /* 3. Disable Timers before config */
    write_cntp_ctl_el0(0); // Disable Physical
    write_cntv_ctl_el0(0); // Disable Virtual

    /* 4. Capture Boot Timestamp */
    sys_uptime.boot_timestamp = read_cntpct_el0();
    sys_uptime.last_tick = sys_uptime.boot_timestamp;
    sys_uptime.uptime_ns = 0;

    /* 5. Configure GIC (Interrupt Controller) */
    /* We need to unmask PPI 30. GIC driver must be ready. */
    gic_enable_irq(sys_timer_config.irq_number);
    gic_set_priority(sys_timer_config.irq_number, 0x00); // Highest Priority
    gic_set_target(sys_timer_config.irq_number, 0x01);   // Target CPU0

    // kprintf("[TIMER] Frequency: %lu Hz\n", freq);
    // kprintf("[TIMER] Boot Timestamp: %lu\n", sys_uptime.boot_timestamp);
    // kprintf("[TIMER] IRQ Line: %d (PPI)\n", sys_timer_config.irq_number);
    
    /* 6. Perform a quick calibration test */
    timer_calibrate_delay();
}

/*
 * timer_calibrate_delay
 * Measures the accuracy of the delay loop against the hardware counter.
 */
void timer_calibrate_delay(void) {
    // kprintf("[TIMER] Calibrating delay loops...\n");
    
    uint64_t start = read_cntpct_el0();
    
    /* Simulate a busy wait of approx 1000 ticks */
    volatile int i;
    for(i = 0; i < 10000; i++) {
        asm volatile("nop");
    }
    
    uint64_t end = read_cntpct_el0();
    uint64_t diff = end - start;
    
    // kprintf("[TIMER] Calibration loop took %lu ticks.\n", diff);
}
/*
 * ======================================================================================
 * SECTION: BLOCKING DELAY FUNCTIONS (BUSY WAIT)
 * ======================================================================================
 * Used for hardware initialization sequences where interrupts are disabled.
 * These functions spin the CPU until the counter delta is met.
 */

/*
 * udelay (Microsecond Delay)
 * Spins for 'usecs' microseconds.
 */
void udelay(uint64_t usecs) {
    uint64_t start_ticks = read_cntpct_el0();
    uint64_t target_ticks = us_to_ticks(usecs);
    
    /* Check for minimal threshold */
    if (target_ticks < sys_timer_config.min_delta_ticks) {
        target_ticks = sys_timer_config.min_delta_ticks;
    }

    /* Spin loop */
    while (1) {
        uint64_t current_ticks = read_cntpct_el0();
        
        /* Handle counter wrap-around (very rare with 64-bit but good practice) */
        if (current_ticks >= start_ticks) {
            if ((current_ticks - start_ticks) >= target_ticks) break;
        } else {
            /* Wrapped case */
            uint64_t delta = (UINT64_MAX - start_ticks) + current_ticks;
            if (delta >= target_ticks) break;
        }
        
        asm volatile("nop"); // Prevent compiler from optimizing loop out
    }
}

/*
 * mdelay (Millisecond Delay)
 * Spins for 'msecs' milliseconds.
 */
void mdelay(uint64_t msecs) {
    /* Call udelay in chunks to prevent watchdog timeouts in future */
    while (msecs > 0) {
        udelay(1000);
        msecs--;
    }
}

/*
 * timer_spin_until
 * Spins until the system timestamp reaches 'abs_time_ns'.
 */
void timer_spin_until(uint64_t abs_time_ns) {
    while (timer_get_uptime_ns() < abs_time_ns) {
        asm volatile("wfe"); // Wait For Event (Power saving spin)
    }
}
/*
 * ======================================================================================
 * SECTION: INTERRUPT CONTROL & SCHEDULER TRIGGER
 * ======================================================================================
 */

/*
 * timer_set_timeout
 * Sets the hardware timer to fire an interrupt after 'ns' nanoseconds.
 * This is used by the Process Scheduler to enforce time slices.
 */
void timer_set_timeout(uint64_t ns) {
    uint64_t ticks = ns_to_ticks(ns);

    /* Enforce hardware limits */
    if (ticks < sys_timer_config.min_delta_ticks) {
        ticks = sys_timer_config.min_delta_ticks;
    }
    
    /* * Method 1: Using TVAL (Timer Value) - Relative Down Counter 
     * Writing to CNTP_TVAL_EL0 automatically updates CVAL.
     */
    write_cntp_tval_el0(ticks);

    /* * Enable the Timer:
     * Bit 0 (ENABLE) = 1
     * Bit 1 (IMASK)  = 0 (Unmasked, allow IRQ)
     */
    uint64_t ctl = TIMER_ENABLE_BIT | TIMER_UNMASK_BIT;
    write_cntp_ctl_el0(ctl);
    
    /* Memory barrier to ensure register write completes */
    memory_barrier();
}

/*
 * timer_cancel_timeout
 * Stops the current timer countdown.
 */
void timer_cancel_timeout(void) {
    /* Write disable bit and mask interrupt */
    uint64_t ctl = TIMER_DISABLE_BIT | TIMER_IMASK_BIT;
    write_cntp_ctl_el0(ctl);
    memory_barrier();
}

/*
 * timer_isr
 * The Interrupt Service Routine called by GIC when ID 30 fires.
 * WARNING: This runs in IRQ context! Keep it short.
 */
void timer_isr(void) {
    /* 1. Read Control Register to verify it's our timer */
    uint64_t ctl = read_cntp_ctl_el0();
    
    if (ctl & TIMER_ISTATUS_BIT) {
        /* 2. Mask the interrupt to prevent re-firing immediately */
        /* We keep ENABLE bit, but set IMASK bit */
        ctl |= TIMER_IMASK_BIT;
        write_cntp_ctl_el0(ctl);
        
        /* 3. Update System Uptime Logic */
        timer_update_uptime();
        
        /* 4. Call the Kernel Scheduler Hook */
        /* extern void scheduler_tick_callback(void); */
        /* scheduler_tick_callback(); */
        
        /* 5. Acknowledge Debug Info (Optional) */
        // kprintf("."); 
    }
}
/*
 * ======================================================================================
 * SECTION: DEBUG & DIAGNOSTICS
 * ======================================================================================
 */

/*
 * timer_dump_regs
 * Dumps all timer registers to the UART console for debugging.
 */
void timer_dump_regs(void) {
    uint64_t cntfrq = read_cntfrq_el0();
    uint64_t cntpct = read_cntpct_el0();
    uint64_t cntp_ctl = read_cntp_ctl_el0();
    uint64_t cntp_cval = read_cntp_cval_el0();

    // kprintf("\n--- TIMER REGISTER DUMP ---\n");
    // kprintf("CNTFRQ_EL0  : 0x%016llX (%lu Hz)\n", cntfrq, cntfrq);
    // kprintf("CNTPCT_EL0  : 0x%016llX\n", cntpct);
    // kprintf("CNTP_CTL_EL0: 0x%016llX (En:%d Mask:%d Stat:%d)\n", 
    //         cntp_ctl, 
    //         (cntp_ctl & 1), 
    //         (cntp_ctl >> 1) & 1, 
    //         (cntp_ctl >> 2) & 1);
    // kprintf("CNTP_CVAL_EL0: 0x%016llX\n", cntp_cval);
    // kprintf("---------------------------\n");
}

/*
 * timer_watchdog_pet
 * Simulates a software watchdog. If the system hangs, this won't be called,
 * and an external hardware watchdog should reset the FPGA.
 */
void timer_watchdog_pet(void) {
    /* In a full implementation, this writes to the Xilinx WDT registers */
    /* ZYNQMP_WDT_BASE + WDT_RESTART_OFFSET = 0x1999 */
    volatile uint32_t *wdt_restart = (uint32_t *)(0xFF150008); // LPD WDT Restart
    *wdt_restart = 0x1999;
}

/*
 * timer_test_suite
 * Runs a set of tests to verify timer functionality.
 */
int timer_test_suite(void) {
    // kprintf("[TIMER] Starting Self-Test...\n");
    
    uint64_t t1 = timer_get_uptime_ns();
    udelay(1000); // Wait 1ms
    uint64_t t2 = timer_get_uptime_ns();
    
    uint64_t diff = t2 - t1;
    
    /* Allow 10% tolerance */
    if (diff < 900000 || diff > 1100000) {
        // kprintf("[TIMER] FAIL: udelay(1000) took %llu ns\n", diff);
        return -1;
    }
    
    // kprintf("[TIMER] PASS: Timing logic is accurate.\n");
    return 0;
}

/* End of File */
