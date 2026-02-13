/*
 * Copyright (C) 2026 PhotonX Technologies.
 * Module: GICv2 Driver (Generic Interrupt Controller)
 * Target: ARM CoreLink GIC-400
 */

#include "platform/zynqmp_hardware.h"
#include "kernel/interrupts.h"

/* Helper Macros for MMIO Access */
#define REG_WRITE(addr, val)   (*(volatile uint32_t *)(addr) = (val))
#define REG_READ(addr)         (*(volatile uint32_t *)(addr))

void gic_init_distributor(void) {
    /* 1. Disable Distributor */
    REG_WRITE(GICD_CTLR, 0);

    /* 2. Set all interrupts to Group 0 */
    for (int i = 0; i < 1024; i += 32) {
        REG_WRITE(GICD_IGROUPR(i / 32), 0);
    }

    /* 3. Set Priority to default (0x80) for all lines */
    for (int i = 0; i < 1024; i += 4) {
        REG_WRITE(GICD_IPRIORITYR(i / 4), 0x80808080);
    }

    /* 4. Target all SPIs to CPU0 */
    for (int i = 32; i < 1024; i += 4) {
        REG_WRITE(GICD_ITARGETSR(i / 4), 0x01010101);
    }

    /* 5. Enable Distributor */
    REG_WRITE(GICD_CTLR, 1);
}

void gic_init_cpu_interface(void) {
    /* 1. Set Priority Mask to allow all interrupts */
    REG_WRITE(GICC_PMR, 0xF0);

    /* 2. Enable CPU Interface */
    REG_WRITE(GICC_CTLR, 1);
}

/*
 * gic_enable_interrupt
 * Unmasks a specific IRQ ID.
 * ID 0-15: SGI (Software Generated)
 * ID 16-31: PPI (Private Peripheral)
 * ID 32+: SPI (Shared Peripheral - FPGA, UART, etc.)
 */
void gic_enable_interrupt(uint32_t id) {
    uint32_t reg_offset = (id / 32) * 4;
    uint32_t bit_offset = id % 32;
    uint32_t val = REG_READ(GICD_ISENABLER(0) + reg_offset);
    
    val |= (1 << bit_offset);
    
    REG_WRITE(GICD_ISENABLER(0) + reg_offset, val);
}

/*
 * gic_acknowledge_interrupt
 * Returns the ID of the pending interrupt.
 */
uint32_t gic_acknowledge_interrupt(void) {
    return REG_READ(GICC_IAR) & 0x3FF; // Mask to 10 bits
}

/*
 * gic_end_of_interrupt
 * Signals completion of handling.
 */
void gic_end_of_interrupt(uint32_t id) {
    REG_WRITE(GICC_EOIR, id);
}
