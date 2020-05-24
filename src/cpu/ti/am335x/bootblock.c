/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <types.h>

#include <arch/cache.h>
#include <bootblock_common.h>

void bootblock_soc_init(void)
{
	uint32_t sctlr;


	mmu_init();

	/* start with mapping everything as strongly ordered. */
	mmu_config_range(0, 4096, DCACHE_OFF);
	mmu_config_range_kb(0x402F0400/1024,
	 		    (0x402F1400-0x402F0400)/1024, DCACHE_WRITETHROUGH);
	dcache_mmu_enable();

	/* enable dcache */
	sctlr = read_sctlr();
	sctlr |= SCTLR_C;
	write_sctlr(sctlr);
}
