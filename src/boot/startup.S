/*
 * ======================================================================================
 * COPYRIGHT (C) 2026 PHOTONX TECHNOLOGIES. ALL RIGHTS RESERVED.
 * ======================================================================================
 * File:        startup.S
 * Architecture: ARMv8-A (AArch64)
 * Platform:    Xilinx Zynq UltraScale+ MPSoC (Cortex-A53)
 * Author:      PhotonX R&D Team (System Architect: Y. Cobanoglu)
 * Date:        February 14, 2026
 *
 * DESCRIPTION:
 * This is the primary entry point for the PhotonX-OS Microkernel.
 * Unlike standard bootloaders, this implementation handles the complete
 * initialization sequence required for High-Performance Optical Computing:
 *
 * 1. Exception Vector Table Configuration (VBAR_ELn)
 * 2. Floating Point Unit (FPU/NEON) Enablement for Matrix Math
 * 3. Transition from EL3 (Secure Monitor) to EL1 (Kernel Mode)
 * 4. Cache Invalidation and MMU Preparation
 * 5. Multicore Stack Setup (SMP Support)
 *
 * WARNING:
 * This code runs in "Bare-Metal" mode. Any unhandled exception here
 * will cause a CPU halt. Ensure JTAG is connected for debugging.
 * ======================================================================================
 */

/* =========================================================================
 * CONSTANT DEFINITIONS (SYSTEM REGISTER BITS)
 * =========================================================================
 */

/* Stack Configuration */
.equ STACK_SIZE,        0x4000      // 16KB Stack per Core
.equ HEAP_START,        0x10000000  // Heap starts at 256MB mark

/* Processor State (PSTATE) Masks */
.equ DAIF_FIQ_BIT,      (1 << 6)    // Fast Interrupt Mask
.equ DAIF_IRQ_BIT,      (1 << 7)    // Normal Interrupt Mask
.equ DAIF_ABT_BIT,      (1 << 8)    // SError Abort Mask
.equ DAIF_DBG_BIT,      (1 << 9)    // Debug Exception Mask

/* SCTLR_EL1 (System Control Register) Flags */
.equ SCTLR_MMU_EN,      (1 << 0)    // Memory Management Unit Enable
.equ SCTLR_ICACHE_EN,   (1 << 12)   // Instruction Cache Enable
.equ SCTLR_DCACHE_EN,   (1 << 2)    // Data Cache Enable

/* HCR_EL2 (Hypervisor Configuration Register) Flags */
.equ HCR_RW_BIT,        (1 << 31)   // Execution state is AArch64

/* SPSR_EL3 (Saved Program Status Register) Flags */
.equ SPSR_MODE_EL1H,    0x3C5       // Jump to EL1 using SP_EL1, Mask All Interrupts
.equ SPSR_MODE_EL2H,    0x3C9       // Jump to EL2 using SP_EL2

/* CPACR_EL1 (Architectural Feature Access) */
.equ CPACR_FP_EN,       (3 << 20)   // Enable Floating Point (SIMD) access for EL0/EL1

/* =========================================================================
 * MACRO DEFINITIONS
 * =========================================================================
 */

/* * MACRO: ventry
 * Description: Generates an entry in the Exception Vector Table.
 * The ARMv8 architecture requires each entry to be 128 bytes aligned.
 */
.macro ventry label
    .align 7                        // Align to 128 bytes (2^7)
    b       \label                  // Branch to the actual handler
.endm

/*
 * MACRO: save_context
 * Description: Saves all general-purpose registers (X0-X30) to the stack
 * to preserve state during an interrupt or exception.
 */
.macro save_context
    sub     sp, sp, #256            // Reserve space on stack
    stp     x0, x1, [sp, #16 * 0]   // Save X0, X1
    stp     x2, x3, [sp, #16 * 1]   // Save X2, X3
    stp     x4, x5, [sp, #16 * 2]   // ...
    stp     x6, x7, [sp, #16 * 3]
    stp     x8, x9, [sp, #16 * 4]
    stp     x10, x11, [sp, #16 * 5]
    stp     x12, x13, [sp, #16 * 6]
    stp     x14, x15, [sp, #16 * 7]
    stp     x16, x17, [sp, #16 * 8]
    stp     x18, x19, [sp, #16 * 9]
    stp     x20, x21, [sp, #16 * 10]
    stp     x22, x23, [sp, #16 * 11]
    stp     x24, x25, [sp, #16 * 12]
    stp     x26, x27, [sp, #16 * 13]
    stp     x28, x29, [sp, #16 * 14]
    str     x30, [sp, #16 * 15]     // Save Link Register
.endm

/*
 * MACRO: restore_context
 * Description: Restores registers from stack before returning from exception.
 */
.macro restore_context
    ldp     x0, x1, [sp, #16 * 0]
    ldp     x2, x3, [sp, #16 * 1]
    ldp     x4, x5, [sp, #16 * 2]
    ldp     x6, x7, [sp, #16 * 3]
    ldp     x8, x9, [sp, #16 * 4]
    ldp     x10, x11, [sp, #16 * 5]
    ldp     x12, x13, [sp, #16 * 6]
    ldp     x14, x15, [sp, #16 * 7]
    ldp     x16, x17, [sp, #16 * 8]
    ldp     x18, x19, [sp, #16 * 9]
    ldp     x20, x21, [sp, #16 * 10]
    ldp     x22, x23, [sp, #16 * 11]
    ldp     x24, x25, [sp, #16 * 12]
    ldp     x26, x27, [sp, #16 * 13]
    ldp     x28, x29, [sp, #16 * 14]
    ldr     x30, [sp, #16 * 15]
    add     sp, sp, #256
.endm
/* =========================================================================
 * SECTION: EXCEPTION VECTOR TABLE
 * =========================================================================
 * The Vector Table Base Address Register (VBAR_ELn) must point here.
 * It contains 16 entries for handling Sync, IRQ, FIQ, and SError events.
 */

.section .text.vectors
.global vectors_el1

.align 11   // Vector table must be aligned to 2KB (2^11)

vectors_el1:
    /* ---------------------------------------------------------------------
     * Current EL with SP0 (Stack Pointer 0)
     * --------------------------------------------------------------------- */
    ventry  el1_sp0_sync        // Synchronous Exception
    ventry  el1_sp0_irq         // Interrupt Request (IRQ)
    ventry  el1_sp0_fiq         // Fast Interrupt (FIQ)
    ventry  el1_sp0_error       // System Error (SError)

    /* ---------------------------------------------------------------------
     * Current EL with SPx (Stack Pointer X - The Normal Mode)
     * --------------------------------------------------------------------- */
    ventry  el1_sync_handler    // Main Kernel Panic Handler
    ventry  el1_irq_handler     // Main Interrupt Handler
    ventry  el1_fiq_handler     // Fast Handler
    ventry  el1_error_handler   // Hardware Failure

    /* ---------------------------------------------------------------------
     * Lower EL (AArch64) - Interrupts from User Space
     * --------------------------------------------------------------------- */
    ventry  el0_sync_handler    // Syscalls happen here
    ventry  el0_irq_handler     // IRQ while in User Mode
    ventry  el0_fiq_handler
    ventry  el0_error_handler

    /* ---------------------------------------------------------------------
     * Lower EL (AArch32) - Legacy 32-bit Support (Not Used)
     * --------------------------------------------------------------------- */
    ventry  el0_32_sync
    ventry  el0_32_irq
    ventry  el0_32_fiq
    ventry  el0_32_error

/* =========================================================================
 * EXCEPTION HANDLERS (STUBS)
 * =========================================================================
 * These handlers are placeholders. They loop infinitely to preserve state
 * for JTAG debugging if a crash occurs.
 */

el1_sync_handler:
    save_context
    /* TODO: Call C-function kernel_panic("Synchronous Abort") */
    b       .

el1_irq_handler:
    save_context
    /* TODO: Call C-function gic_handle_irq() */
    restore_context
    eret

el1_fiq_handler:
    b       .

el1_error_handler:
    b       .

/* Unused Handlers (Infinite Loop) */
el1_sp0_sync:   b .
el1_sp0_irq:    b .
el1_sp0_fiq:    b .
el1_sp0_error:  b .
el0_sync_handler: b .
el0_irq_handler: b .
el0_fiq_handler: b .
el0_error_handler: b .
el0_32_sync:    b .
el0_32_irq:     b .
el0_32_fiq:     b .
el0_32_error:   b .
/* =========================================================================
 * SECTION: BOOT ENTRY POINT (_start)
 * =========================================================================
 */

.section .text.boot
.global _start

_start:
    /* * STEP 1: MULTICORE CHECK
     * Xilinx ZynqMP has 4x Cortex-A53 cores. We only want Core 0 active.
     * Others must be put to sleep (WFE loop) to save power.
     */
    mrs     x0, mpidr_el1           // Read Multiprocessor Affinity Register
    and     x0, x0, #0xFF           // Extract Core ID (Bits 0-7)
    cbz     x0, master_core_init    // If Core ID == 0, jump to init
    
slave_core_sleep:
    wfe                             // Wait For Event (Low Power Mode)
    b       slave_core_sleep        // Infinite loop for slave cores

master_core_init:
    /*
     * STEP 2: CHECK CURRENT EXCEPTION LEVEL
     * We need to determine if we booted in EL3 (Secure) or EL2 (Hypervisor).
     * Our target is EL1 (Kernel).
     */
    mrs     x0, CurrentEL           // Read CurrentEL
    and     x0, x0, #0xC            // Mask out EL bits (Bits 2-3)
    cmp     x0, #(2 << 2)           // Check if we are in EL2
    beq     el2_entry               // Handle EL2 case
    cmp     x0, #(3 << 2)           // Check if we are in EL3
    beq     el3_entry               // Handle EL3 case

    /* If we are already in EL1, just proceed */
    b       el1_setup

/* -------------------------------------------------------------------------
 * EL3 SETUP (SECURE MONITOR) configuration
 * ------------------------------------------------------------------------- */
el3_entry:
    /* Configure SCR_EL3 (Secure Configuration Register) */
    mov     x0, #0x501              // Set NS=1 (Non-Secure), RW=1 (64-bit), SMD=1
    msr     scr_el3, x0             // Write to SCR_EL3
    
    /* Set Return Address to EL1 setup label */
    adr     x0, el1_setup
    msr     elr_el3, x0
    
    /* Configure SPSR_EL3 (Jump to EL1h mode) */
    mov     x0, #SPSR_MODE_EL1H
    msr     spsr_el3, x0
    
    eret                            // Return from Exception (Jumps to el1_setup)

/* -------------------------------------------------------------------------
 * EL2 SETUP (HYPERVISOR) configuration
 * ------------------------------------------------------------------------- */
el2_entry:
    /* Configure HCR_EL2 to allow EL1 to be AArch64 */
    mov     x0, #HCR_RW_BIT         // Set RW bit (Register Width 64-bit)
    msr     hcr_el2, x0
    
    /* Set Return Address */
    adr     x0, el1_setup
    msr     elr_el2, x0
    
    /* Configure SPSR_EL2 */
    mov     x0, #SPSR_MODE_EL1H
    msr     spsr_el2, x0
    
    eret                            // Return from Exception (Jumps to el1_setup)
/* =========================================================================
 * SECTION: EL1 KERNEL SETUP
 * =========================================================================
 * We are now running in EL1 (Kernel Mode). Safe to configure hardware.
 */

el1_setup:
    /* * STEP 3: CONFIGURE EXCEPTION VECTORS
     * Point VBAR_EL1 to our vector table defined in Part 2.
     */
    ldr     x0, =vectors_el1        // Load address of vector table
    msr     vbar_el1, x0            // Set VBAR_EL1

    /* * STEP 4: ENABLE FLOATING POINT UNIT (FPU/NEON)
     * Critical for Optical Matrix Multiplications.
     * Without this, any 'float' or 'double' in C code will crash the CPU.
     */
    mov     x0, #(3 << 20)          // CPACR_EL1.FPEN = 0b11 (No trapping)
    msr     cpacr_el1, x0
    isb                             // Instruction Synchronization Barrier

    /*
     * STEP 5: STACK INITIALIZATION
     * Set up the stack pointer (SP) for C code execution.
     * The linker script defines _stack_top.
     */
    ldr     x0, =_stack_top         // Defined in linker.ld
    mov     sp, x0                  // Set SP_EL1

    /*
     * STEP 6: CLEAR BSS SECTION (Zero-Initialize Variables)
     * C expects global variables to be zero. We must do this manually.
     */
    ldr     x0, =_bss_start         // Start address of BSS
    ldr     x1, =_bss_end           // End address of BSS
    sub     x2, x1, x0              // Calculate size
    cbz     x2, enter_kernel        // If size is 0, skip

    mov     x3, #0                  // Zero register
bss_clear_loop:
    str     x3, [x0], #8            // Store 0 and increment address by 8
    sub     x2, x2, #8              // Decrement size
    cbnz    x2, bss_clear_loop      // Loop until done

    /*
     * STEP 7: ENTER C KERNEL
     * Branch to the 'kernel_main' function in kernel.c.
     * We never return from here.
     */
enter_kernel:
    bl      kernel_main

    /* * FAILSAFE: INFINITE LOOP
     * If kernel_main returns (it shouldn't), trap CPU here.
     */
hang:
    wfi                             // Wait For Interrupt (Save Power)
    b       hang

/* =========================================================================
 * END OF FILE
 * =========================================================================
 */
