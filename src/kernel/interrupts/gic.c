/*
 * ======================================================================================
 * COPYRIGHT (C) 2026 PHOTONX TECHNOLOGIES. ALL RIGHTS RESERVED.
 * ======================================================================================
 * File:        gic.c
 * Module:      GICv2 Driver Implementation
 * Platform:    Xilinx Zynq UltraScale+ MPSoC
 * Author:      PhotonX R&D Team
 *
 * DESCRIPTION:
 * Implements the low-level driver for the GIC-400 Interrupt Controller.
 * This driver is critical for:
 * - Receiving 'Calculation Done' signals from HOCS Optical Unit.
 * - Handling UART input characters.
 * - Managing System Timer ticks for the Scheduler.
 * ======================================================================================
 */

#include "drivers/gic_v2.h"
#include "lib/kprintf.h"  // Assuming we have a kernel printf
#include "platform/zynqmp_hardware.h"

/* Helper Macros for Memory Mapped I/O */
#define MMIO_READ32(addr)       (*(volatile uint32_t *)(addr))
#define MMIO_WRITE32(addr, val) (*(volatile uint32_t *)(addr) = (val))

/*
 * ======================================================================================
 * FUNCTION: gic_dist_init
 * DESCRIPTION:
 * Initializes the GIC Distributor. This is global for all CPUs.
 * It disables interrupts, sets default priorities, and routes everything to CPU0.
 * ======================================================================================
 */
static void gic_dist_init(void) {
    uint32_t i;
    uint32_t num_irqs;

    /* 1. Disable the Distributor during configuration */
    MMIO_WRITE32(GICD_CTLR, 0x00000000);

    /* 2. Determine number of implemented IRQ lines */
    /* GICD_TYPER bits [4:0] indicate (number of lines / 32) - 1 */
    uint32_t type = MMIO_READ32(GICD_TYPER);
    num_irqs = 32 * ((type & 0x1F) + 1);

    if (num_irqs > MAX_IRQS) {
        num_irqs = MAX_IRQS;
    }

    /* 3. Configure all SPIs (Shared Peripheral Interrupts) */
    /* Loop through all interrupt lines from 32 up to max */
    for (i = IRQ_SPI_START; i < num_irqs; i += 32) {
        /* Disable the interrupts first */
        MMIO_WRITE32(GICD_ICENABLER(i / 32), 0xFFFFFFFF);

        /* Set all to be Level Sensitive (0) by default, or Edge (1) if needed */
        /* For simplified HOCS implementation, we assume Level Sensitive */
        // MMIO_WRITE32(GICD_ICFGR(i / 16), 0x00000000); 
    }

    /* 4. Set Priority for ALL interrupts to default (Medium) */
    /* We iterate 4 interrupts at a time (4 * 8 bits = 32 bits register) */
    for (i = 0; i < num_irqs; i += 4) {
        /* Set priority to 0x80 (128) for all */
        MMIO_WRITE32(GICD_IPRIORITYR(i / 4), 0x80808080);
    }

    /* 5. Route ALL interrupts to CPU0 (Target Register) */
    /* This is critical for Bare-Metal on Master Core */
    /* SPIs start at ID 32. Registers map 4 targets per 32-bit word */
    for (i = IRQ_SPI_START; i < num_irqs; i += 4) {
        /* 0x01 = CPU0, 0x02 = CPU1 ... repeated 4 times */
        MMIO_WRITE32(GICD_ITARGETSR(i / 4), 0x01010101);
    }

    /* 6. Configure Security (Group 0 vs Group 1) */
    /* We put everything in Group 0 (Secure) for now */
    for (i = 0; i < num_irqs; i += 32) {
        MMIO_WRITE32(GICD_IGROUPR(i / 32), 0x00000000);
    }

    /* 7. Re-Enable the Distributor */
    MMIO_WRITE32(GICD_CTLR, GICD_CTLR_ENABLE);
}
/*
 * ======================================================================================
 * FUNCTION: gic_cpu_init
 * DESCRIPTION:
 * Initializes the GIC CPU Interface for the CURRENTLY running core.
 * Each core must call this function individually.
 * ======================================================================================
 */
static void gic_cpu_init(void) {
    /* 1. Set Priority Mask to 0xF0 (Allow all priorities except lowest) */
    /* If an interrupt has priority > 0xF0, it will be masked. */
    MMIO_WRITE32(GICC_PMR, 0xF0);

    /* 2. Set Binary Point to 0 (No sub-priority split) */
    MMIO_WRITE32(GICC_BPR, 0x00);

    /* 3. Enable CPU Interface */
    MMIO_WRITE32(GICC_CTLR, GICC_CTLR_ENABLE);
}

/*
 * ======================================================================================
 * FUNCTION: gic_init (GLOBAL ENTRY POINT)
 * ======================================================================================
 */
void gic_init(void) {
    /* Initialize the global distributor first */
    gic_dist_init();
    
    /* Initialize the interface for this specific CPU */
    gic_cpu_init();
    
    // kprintf("[GICv2] Initialization Complete. Routing to CPU0.\n");
}

/*
 * ======================================================================================
 * DRIVER API: Enable / Disable / Acknowledge
 * ======================================================================================
 */

void gic_enable_irq(uint32_t irq_id) {
    /* Calculate register offset and bit position */
    uint32_t reg_offset = (irq_id / 32);
    uint32_t bit_mask   = (1 << (irq_id % 32));

    /* Write to Set-Enable Register */
    /* Writing 1 enables, writing 0 has no effect */
    MMIO_WRITE32(GICD_ISENABLER(reg_offset), bit_mask);
}

void gic_disable_irq(uint32_t irq_id) {
    uint32_t reg_offset = (irq_id / 32);
    uint32_t bit_mask   = (1 << (irq_id % 32));

    /* Write to Clear-Enable Register */
    MMIO_WRITE32(GICD_ICENABLER(reg_offset), bit_mask);
}

void gic_set_priority(uint32_t irq_id, uint8_t priority) {
    uint32_t reg_offset = (irq_id / 4);
    uint32_t bit_shift  = (irq_id % 4) * 8;
    
    /* Read-Modify-Write sequence */
    uint32_t val = MMIO_READ32(GICD_IPRIORITYR(reg_offset));
    
    /* Clear old priority (8 bits) */
    val &= ~(0xFF << bit_shift);
    
    /* Set new priority */
    val |= ((uint32_t)priority << bit_shift);
    
    MMIO_WRITE32(GICD_IPRIORITYR(reg_offset), val);
}

/*
 * ======================================================================================
 * FUNCTION: gic_handle_irq (CRITICAL PATH)
 * DESCRIPTION:
 * This function is called directly from the Assembly Vector Table (el1_irq_handler).
 * It identifies the source of the interrupt and dispatches it.
 * ======================================================================================
 */
void gic_handle_irq_c_handler(void) {
    /* 1. Read Interrupt Acknowledge Register (IAR) */
    /* This tells us WHICH device caused the interrupt */
    uint32_t iar = MMIO_READ32(GICC_IAR);
    uint32_t irq_id = iar & 0x3FF; // Extract 10-bit ID

    /* Check for Spurious Interrupts (ID 1023) */
    if (irq_id == 1023) {
        return; // Noise on the line, ignore.
    }

    /* 2. Dispatch Handling Logic */
    /* In a real OS, we would look up a 'Handler Table' here. */
    /* For HOCS, we check specific IDs manually. */
    
    // kprintf("[IRQ] Received Interrupt ID: %d\n", irq_id);

    /* Example: HOCS Optical Done Signal (Assume ID 120 from PL) */
    if (irq_id == 120) {
        // hocs_optical_isr(); 
    }
    /* Example: UART Receive Interrupt (ID 53 for UART0) */
    else if (irq_id == 53) {
        // uart_rx_isr();
    }

    /* 3. End of Interrupt (EOI) */
    /* Signal the GIC that we are finished, so it can send more IRQs */
    MMIO_WRITE32(GICC_EOIR, iar);
}
