/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <memlayout.h>

#include <arch/header.ld>

SECTIONS
{
	DRAM_START(0x402f0400)
	BOOTBLOCK(0x402f0400, 12K)
	ROMSTAGE(0x402f3400, 28K)
	FMAP_CACHE(0x402FA400, 2K)
	STACK(0x4030be00, 4K)
	RAMSTAGE(0x80200000, 70K)

	/* TODO: Implement MMU support and move TTB to a better location. */
	TTB(0x81000000, 16K)

#ifdef OMAP_HEADER
	.header : {
		*(.header);
	} : to_load

	/DISCARD/ : {
		*(*)
	}
#endif
}
