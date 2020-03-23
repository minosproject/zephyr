/*
 * Copyright 2019 Broadcom
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <arch/arm/aarch64/arm_mmu.h>
#include <dts_fixup.h>

#define SZ_1K	1024

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("GIC0",
			      DT_INST_0_ARM_GIC_BASE_ADDRESS_0,
			      DT_INST_0_ARM_GIC_SIZE_0,
			      MT_DEVICE_nGnRnE | MT_RW | MT_SECURE),
	MMU_REGION_FLAT_ENTRY("GIC1",
			      DT_INST_0_ARM_GIC_BASE_ADDRESS_1,
			      DT_INST_0_ARM_GIC_SIZE_1,
			      MT_DEVICE_nGnRnE | MT_RW | MT_SECURE),
	MMU_REGION_FLAT_ENTRY("GIC2",
			      DT_INST_0_ARM_GIC_BASE_ADDRESS_2,
			      DT_INST_0_ARM_GIC_SIZE_2,
			      MT_DEVICE_nGnRnE | MT_RW | MT_SECURE),
	MMU_REGION_FLAT_ENTRY("GIC3",
			      DT_INST_0_ARM_GIC_BASE_ADDRESS_3,
			      DT_INST_0_ARM_GIC_SIZE_3,
			      MT_DEVICE_nGnRnE | MT_RW | MT_SECURE),

	MMU_REGION_FLAT_ENTRY("VM_CONSOLE",
			      DT_INST_0_MINOS_VM_CONSOLE_BASE_ADDRESS,
			      DT_INST_0_MINOS_VM_CONSOLE_SIZE,
			      MT_DEVICE_nGnRnE | MT_RW | MT_SECURE),
#if 0
	MMU_REGION_FLAT_ENTRY("UART",
			      DT_INST_0_ARM_PL011_BASE_ADDRESS,
			      DT_INST_0_ARM_PL011_SIZE,
			      MT_DEVICE_nGnRnE | MT_RW | MT_SECURE),
#endif
	MMU_REGION_FLAT_ENTRY("VMBOX_CON",
			      DT_INST_0_MINOS_VMBOX_CONTROLLER_BASE_ADDRESS,
			      DT_INST_0_MINOS_VMBOX_CONTROLLER_SIZE,
			      MT_DEVICE_nGnRnE | MT_RW | MT_SECURE),

	MMU_REGION_FLAT_ENTRY("SRAM",
			      CONFIG_SRAM_BASE_ADDRESS,
			      CONFIG_SRAM_SIZE * SZ_1K,
			      MT_NORMAL | MT_RW | MT_SECURE),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
