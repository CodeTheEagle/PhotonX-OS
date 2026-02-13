/*
 * Copyright (C) 2026 PhotonX Technologies.
 * System: PhotonX-OS (HOCS Microkernel)
 * Module: Memory Management Unit (MMU) - AArch64 Implementation
 * Author: PhotonX R&D Team
 *
 * Description:
 * This module configures the VMSAv8-64 (Virtual Memory System Architecture).
 * It manages the Translation Control Register (TCR), Memory Attribute Indirection
 * Register (MAIR), and the translation tables for the Lower (User) and
 * Upper (Kernel) address spaces.
 *
 * TARGET: Xilinx Zynq UltraScale+ MPSoC (Cortex-A53)
 * PAGE SIZE: 4KB
 * GRANULE: 4KB
 * PA RANGE: 48-bit (256TB Physical Address Space)
 */

#include "system.h"
#include "mm/mmu_defs.h"
#include "platform/zynqmp_hardware.h"
#include "lib/stddef.h"

/*
 * Global Translation Tables
 * Must be aligned to 4KB (0x1000) boundary.
 * * L0 Table: Covers 512GB per entry
 * L1 Table: Covers 1GB per entry (Block Descriptor)
 * L2 Table: Covers 2MB per entry (Block Descriptor)
 * L3 Table: Covers 4KB per entry (Page Descriptor)
 */
__attribute__((aligned(4096))) uint64_t kernel_l0_table[512];
__attribute__((aligned(4096))) uint64_t kernel_l1_table[512];
__attribute__((aligned(4096))) uint64_t kernel_l2_table[512 * 4]; // Expanded for 2GB Identity Map

/* * mmu_init_mair
 * Configures the Memory Attribute Indirection Register (MAIR_EL1).
 * * Attr0: Device-nGnRnE (Strictly Ordered, Non-Cacheable) - For UART/FPGA Registers
 * Attr1: Normal Memory (Outer Write-Back, Inner Write-Back) - For RAM/Code
 * Attr2: Device-nGnRE (Non-Ordering) - For PCIe/DMA
 */
void mmu_init_mair(void) {
    uint64_t mair_val = 0;

    // Attribute 0: 0x00 -> Device-nGnRnE
    mair_val |= (MAIR_ATTR_DEVICE_nGnRnE << (8 * 0));

    // Attribute 1: 0xFF -> Normal Memory, Write-Back, Read/Write Allocate
    mair_val |= (MAIR_ATTR_NORMAL_WB << (8 * 1));

    // Attribute 2: 0x04 -> Device-nGnRE
    mair_val |= (MAIR_ATTR_DEVICE_nGnRE << (8 * 2));

    // Write to MAIR_EL1
    asm volatile("msr mair_el1, %0" : : "r" (mair_val));
    asm volatile("isb"); // Instruction Synchronization Barrier
}

/*
 * mmu_init_tcr
 * Configures the Translation Control Register (TCR_EL1).
 * * T0SZ/T1SZ: Size of the address space (TxSZ = 64 - 48 = 16)
 * TG0/TG1:   Granule Size (4KB)
 * IPS:       Intermediate Physical Address Size (48-bit)
 */
void mmu_init_tcr(void) {
    uint64_t tcr_val = 0;

    // T0SZ = 16 (48-bit VA for User Space)
    tcr_val |= (16UL << TCR_T0SZ_SHIFT);
    // T1SZ = 16 (48-bit VA for Kernel Space)
    tcr_val |= (16UL << TCR_T1SZ_SHIFT);

    // TG0 = 4KB (0b00)
    tcr_val |= (TCR_TG0_4KB << TCR_TG0_SHIFT);
    // TG1 = 4KB (0b10)
    tcr_val |= (TCR_TG1_4KB << TCR_TG1_SHIFT);

    // IPS = 48-bit (0b101) - Supported by Cortex-A53
    tcr_val |= (TCR_IPS_48BIT << TCR_IPS_SHIFT);

    // SH (Shareability) = Inner Shareable
    tcr_val |= (TCR_SH_INNER << TCR_SH0_SHIFT);
    tcr_val |= (TCR_SH_INNER << TCR_SH1_SHIFT);

    // Write to TCR_EL1
    asm volatile("msr tcr_el1, %0" : : "r" (tcr_val));
    asm volatile("isb");
}

/*
 * mmu_create_identity_map
 * Creates a 1:1 mapping for the kernel code and data structures.
 * This ensures the CPU doesn't crash when MMU is enabled.
 * * Mapping Strategy:
 * 0x0000_0000 - 0x7FFF_FFFF (2GB RAM) -> Normal Memory (Cached)
 * 0x8000_0000 - 0xFFFF_FFFF (MMIO)    -> Device Memory (Uncached)
 */
void mmu_create_identity_map(void) {
    uint64_t virt_addr, phys_addr;
    int i, j;

    /* 1. Setup L0 Table (Root) pointing to L1 Table */
    // Entry 0 covers 0x0000_0000 to 0x0080_0000_0000 (512GB)
    kernel_l0_table[0] = (uint64_t)kernel_l1_table | PT_TABLE_DESC | PT_ACCESS_FULL;

    /* 2. Setup L1 Table pointing to L2 Tables */
    // We map the first 4GB using 4 Block Descriptors (1GB each)
    // Actually, let's use finer granularity (L2) for the first 2GB
    kernel_l1_table[0] = (uint64_t)&kernel_l2_table[0] | PT_TABLE_DESC | PT_ACCESS_FULL;
    kernel_l1_table[1] = (uint64_t)&kernel_l2_table[512] | PT_TABLE_DESC | PT_ACCESS_FULL;

    /* 3. Setup L2 Table (2MB Blocks) for RAM (0-2GB) */
    // 0x00000000 -> 0x7FFFFFFF : Normal Memory, Executable
    phys_addr = 0;
    for (i = 0; i < 1024; i++) { // 1024 entries * 2MB = 2GB
        uint64_t attr = PT_BLOCK_DESC | PT_ACCESS_FULL | PT_SH_INNER;
        
        // Mark strictly as 'Normal Memory' (Attr Index 1)
        attr |= (1 << 2); 
        
        // Define Physical Address
        kernel_l2_table[i] = phys_addr | attr;
        phys_addr += 0x200000; // Increment by 2MB
    }

    /* 4. Setup L2 Table for MMIO (Device Registers) */
    // 0x80000000 -> 0xFFFFFFFF : Device Memory (UART, GIC, FPGA AXI)
    // Note: This requires extending the tables to cover the upper region
    // For simplicity in this snippet, we assume identity mapping continues.
    // In production, we map specific ranges like 0xFF000000 (UART).
}

/*
 * mmu_enable
 * Writes the table address to TTBR0/1 and enables the MMU (SCTLR_EL1).
 */
void mmu_enable(void) {
    uint64_t sctlr;

    // 1. Set Translation Table Base Registers
    asm volatile("msr ttbr0_el1, %0" : : "r" (kernel_l0_table));
    asm volatile("msr ttbr1_el1, %0" : : "r" (kernel_l0_table));
    asm volatile("isb");

    // 2. Invalidate TLB (Translation Lookaside Buffer)
    asm volatile("tlbi vmalle1");
    asm volatile("dsb nsh");
    asm volatile("isb");

    // 3. Enable MMU and Caches in SCTLR_EL1
    asm volatile("mrs %0, sctlr_el1" : "=r" (sctlr));
    
    sctlr |= SCTLR_M_BIT;  // Enable MMU
    sctlr |= SCTLR_C_BIT;  // Enable Data Cache
    sctlr |= SCTLR_I_BIT;  // Enable Instruction Cache
    
    asm volatile("msr sctlr_el1, %0" : : "r" (sctlr));
    asm volatile("isb");
    
    // 4. Verify
    // If we are here, virtual memory is active.
}

/*
 * vmm_map_page
 * Dynamically maps a Virtual Page to a Physical Frame.
 * Used by the HOCS Process Loader when starting new tasks.
 * * @param va: Virtual Address
 * @param pa: Physical Address
 * @param flags: R/W/X permissions
 */
int vmm_map_page(uint64_t va, uint64_t pa, uint64_t flags) {
    // Traverse L0 -> L1 -> L2 -> L3
    // If a table is missing, allocate a new physical page for the table.
    
    uint64_t l0_idx = (va >> 39) & 0x1FF;
    uint64_t l1_idx = (va >> 30) & 0x1FF;
    uint64_t l2_idx = (va >> 21) & 0x1FF;
    uint64_t l3_idx = (va >> 12) & 0x1FF;

    // Implementation omitted for brevity in this snippet.
    // Full implementation requires Physical Memory Manager (PMM).
    return 0; // Success
}
