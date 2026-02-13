/*
 * Copyright (C) 2026 PhotonX Technologies.
 * File: mmu_defs.h
 * Description: 
 * Register bit definitions for ARMv8-A MMU (VMSAv8-64).
 * Used by assembly bootloader and C kernel.
 */

#ifndef _PHOTONX_MMU_DEFS_H_
#define _PHOTONX_MMU_DEFS_H_

/* Block/Page Descriptor Types */
#define PT_INVALID              0x0
#define PT_BLOCK_DESC           0x1     // Block (2MB or 1GB)
#define PT_TABLE_DESC           0x3     // Table (Next Level)
#define PT_PAGE_DESC            0x3     // Page (4KB)

/* Access Permissions (AP Bits) */
#define PT_ACCESS_PRIV_RW       (0x0 << 6) // Kernel: RW, User: None
#define PT_ACCESS_FULL          (0x1 << 6) // Kernel: RW, User: RW
#define PT_ACCESS_PRIV_RO       (0x2 << 6) // Kernel: RO, User: None
#define PT_ACCESS_RO            (0x3 << 6) // Kernel: RO, User: RO

/* Shareability (SH Bits) */
#define PT_SH_NON               (0x0 << 8)
#define PT_SH_OUTER             (0x2 << 8)
#define PT_SH_INNER             (0x3 << 8)

/* Execute Never (XN Bits) */
#define PT_UXN                  (1UL << 54) // User Execute Never
#define PT_PXN                  (1UL << 53) // Privileged Execute Never

/* MAIR Attributes Indices */
#define MAIR_ATTR_DEVICE_nGnRnE 0x00
#define MAIR_ATTR_NORMAL_WB     0xFF
#define MAIR_ATTR_DEVICE_nGnRE  0x04

/* TCR (Translation Control Register) Flags */
#define TCR_T0SZ_SHIFT          0
#define TCR_T1SZ_SHIFT          16
#define TCR_TG0_SHIFT           14
#define TCR_TG1_SHIFT           30
#define TCR_IPS_SHIFT           32
#define TCR_SH0_SHIFT           12
#define TCR_SH1_SHIFT           28

#define TCR_TG0_4KB             0x0
#define TCR_TG1_4KB             0x2
#define TCR_IPS_48BIT           0x5
#define TCR_SH_INNER            0x3

/* SCTLR (System Control Register) Flags */
#define SCTLR_M_BIT             (1 << 0)  // MMU Enable
#define SCTLR_C_BIT             (1 << 2)  // Data Cache Enable
#define SCTLR_I_BIT             (1 << 12) // Instruction Cache Enable

#endif /* _PHOTONX_MMU_DEFS_H_ */
