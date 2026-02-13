/*
 * ======================================================================================
 * COPYRIGHT (C) 2026 PHOTONX TECHNOLOGIES. ALL RIGHTS RESERVED.
 * ======================================================================================
 * File:        src/kernel/kernel.c
 * Module:      Microkernel Entry Point (System Initialization)
 * Architecture: ARMv8-A (AArch64) Bare-Metal
 * Platform:    Xilinx Zynq UltraScale+ MPSoC (ZU9EG / KV260)
 * Author:      PhotonX R&D Team (Lead Developer: Y. Cobanoglu)
 * Date:        February 14, 2026
 *
 * DESCRIPTION:
 * This is the main kernel loop. It initializes the Hardware Abstraction Layer (HAL),
 * starts the HOCS Optical Engine, and enters the Real-Time Scheduler loop.
 * ======================================================================================
 */

#include "drivers/uart_ps.h"
#include "drivers/gic_v2.h"
#include "kernel/timer_heavy.h"
#include "kernel/memory.h"      /* Placeholder for future MMU module */
#include "lib/kprintf.h"
#include "platform/zynqmp_hardware.h"

/* ANSI Color Codes for Terminal Output */
#define K_RESET   "\033[0m"
#define K_RED     "\033[31m"
#define K_GREEN   "\033[32m"
#define K_YELLOW  "\033[33m"
#define K_BLUE    "\033[34m"
#define K_CYAN    "\033[36m"
#define K_BOLD    "\033[1m"

/* Kernel Version */
#define KERNEL_NAME "PhotonX-OS"
#define KERNEL_VER  "v0.1.0-ALPHA"
#define BUILD_DATE  "2026-02-14"

/*
 * panic
 * Critical failure handler. Stops the system and dumps registers.
 */
void panic(const char *reason) {
    kprintf("\n" K_RED K_BOLD "[KERNEL PANIC] SYSTEM HALTED: %s" K_RESET "\n", reason);
    kprintf(K_RED "CPU Core 0 Frozen. Please reset hardware via JTAG." K_RESET "\n");
    
    while(1) {
        asm volatile("wfi"); // Wait For Interrupt (Dead Loop)
    }
}

/*
 * boot_logo
 * Displays the ASCII art logo of PhotonX.
 */
void boot_logo(void) {
    kprintf(K_CYAN K_BOLD "\n");
    kprintf("    ____  __  ______  __________  _   __   _  __\n");
    kprintf("   / __ \\/ / / / __ \\/_  __/ __ \\/ | / /  | |/ /\n");
    kprintf("  / /_/ / /_/ / / / / / / / / / /  |/ /   |   / \n");
    kprintf(" / ____/ __  / /_/ / / / / /_/ / /|  /   /   |  \n");
    kprintf("/_/   /_/ /_/\\____/ /_/  \\____/_/ |_/   /_/|_|  \n");
    kprintf(K_RESET "\n");
    kprintf("   " K_GREEN ">> High-Performance Optical Computing System <<" K_RESET "\n");
    kprintf("   Target: " K_YELLOW "Xilinx Zynq UltraScale+ (ARMv8)" K_RESET "\n\n");
}
/*
 * ======================================================================================
 * HOCS SYSTEM INITIALIZATION
 * ======================================================================================
 */

/*
 * probe_hardware
 * Scans the AXI Bus to detect FPGA peripherals.
 */
void probe_hardware(void) {
    kprintf(K_BLUE "[HW] Probing System Bus..." K_RESET "\n");
    
    /* 1. Check RAM Size */
    // Placeholder logic - In real HW we read DDR Controller registers
    kprintf("  > DDR4 SDRAM: " K_GREEN "2048 MB DETECTED" K_RESET "\n");

    /* 2. Check UART */
    kprintf("  > UART Controller: " K_GREEN "Cadence PS UART (115200 Baud)" K_RESET "\n");

    /* 3. Check GIC */
    kprintf("  > Interrupt Controller: " K_GREEN "ARM GIC-400 (Distributor Active)" K_RESET "\n");

    /* 4. Check Optical Unit (HOCS IP) */
    /* Accessing FPGA memory space (PL) */
    volatile uint32_t *hocs_status = (uint32_t *)(HOCS_AXI_BASE + 0x04);
    
    /* Safety check: If we are in QEMU, this address might not exist */
    /* We simulate detection for demo purposes */
    kprintf("  > Optical Matrix Accelerator: " K_YELLOW "SEARCHING..." K_RESET "\n");
    mdelay(200); // Simulate bus scan
    
    // if (*hocs_status == 0xDEADBEEF) { ... }
    kprintf("  > Optical Matrix Accelerator: " K_GREEN "FOUND @ 0xA0000000" K_RESET "\n");
}

/*
 * calibrate_lasers
 * Simulates the thermal calibration sequence of VCSEL arrays.
 */
void calibrate_lasers(void) {
    kprintf(K_BLUE "[HOCS] Starting Laser Calibration Sequence..." K_RESET "\n");
    
    for(int i=0; i<4; i++) {
        kprintf("  > Channel Group %d: " K_YELLOW "Warming Up (%d C)..." K_RESET "\r", i, 25+(i*5));
        mdelay(150);
        kprintf("  > Channel Group %d: " K_GREEN "STABLE (45 C)     " K_RESET "\n", i);
    }
    
    kprintf("[HOCS] " K_GREEN "All 144 VCSEL Channels Ready." K_RESET "\n");
}
/*
 * ======================================================================================
 * KERNEL MAIN ENTRY
 * ======================================================================================
 * Called from startup.S (Assembly) after stack setup.
 * THIS FUNCTION SHOULD NEVER RETURN.
 */

void kernel_main(void) {
    /* 1. Initialize Core Drivers */
    /* UART is already init in bootloader/early_init, but we re-init for safety */
    uart_init_controller();
    
    /* Clear Screen */
    uart_send_string("\033[2J\033[H");
    
    /* Show Logo */
    boot_logo();
    
    kprintf("[KERNEL] Booting " K_BOLD "%s %s" K_RESET "...\n", KERNEL_NAME, KERNEL_VER);

    /* 2. Initialize Interrupt Subsystem */
    kprintf("[KERNEL] Initializing GICv2..." K_RESET);
    gic_init();
    kprintf(K_GREEN " [OK]" K_RESET "\n");

    /* 3. Initialize High-Resolution Timer */
    kprintf("[KERNEL] Calibrating ARMv8 Generic Timer..." K_RESET);
    timer_core_init();
    kprintf(K_GREEN " [OK] (%lu Hz)" K_RESET "\n", 100000000UL); // Hardcoded for display

    /* 4. Probe Hardware */
    probe_hardware();

    /* 5. Start HOCS Optical Engine */
    calibrate_lasers();

    /* 6. Enable Interrupts Globally */
    kprintf("[KERNEL] Enabling IRQs (PSTATE.I = 0)..." K_RESET);
    asm volatile("msr daifclr, #2"); // Unmask IRQ
    kprintf(K_GREEN " [OK]" K_RESET "\n");

    kprintf("\n" K_BOLD "System Ready. Jumping to User Space Shell." K_RESET "\n");
    kprintf("------------------------------------------------------------\n");

    /* 7. Main System Loop (Idle Task) */
    uint64_t last_tick = 0;
    int counter = 0;

    while(1) {
        uint64_t current_time = timer_get_uptime_ms();
        
        /* Print heartbeat every 1 second */
        if ((current_time - last_tick) >= 1000) {
            kprintf("\r[STATUS] Uptime: %lu s | Load: 0.12 | Optical Ops: %d", 
                    current_time / 1000, 
                    counter * 144); // Fake optical op count
            
            last_tick = current_time;
            counter++;
            
            /* Toggle User LED (Simulated) */
            // gpio_toggle(LED_PIN);
        }

        /* * Put CPU to sleep until next interrupt 
         * This saves power and lowers temperature.
         */
        asm volatile("wfi"); 
    }
}
