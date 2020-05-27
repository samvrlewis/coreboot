/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <memlayout.h>

#include <arch/header.ld>

SECTIONS
{
	DRAM_START(0x402f0400)
	BOOTBLOCK(0x402f0400, 13K)
	FMAP_CACHE(0x402F3800, 2K)
	TTB(0x402F4000, 16K)
	/*TTB_SUBTABLES(0x402F8000, 1K)*/

	ROMSTAGE(0x402F8400, 28K)

	STACK(0x4030be00, 4K)
	RAMSTAGE(0x80200000, 70K)

	/* TODO: Implement MMU support and move TTB to a better location. */
	

#ifdef OMAP_HEADER
	.header : {
		*(.header);
	} : to_load

	/DISCARD/ : {
		*(*)
	}
#endif
}
