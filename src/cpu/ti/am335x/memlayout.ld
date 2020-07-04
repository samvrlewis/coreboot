/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <memlayout.h>

#include <arch/header.ld>

SECTIONS
{
	SRAM_START(0x402f0400)
	BOOTBLOCK(0x402f0400, 13K)
	FMAP_CACHE(0x402F3800, 2K)
	TTB(0x402F4000, 16K)
	/*TTB_SUBTABLES(0x402F8000, 1K)*/

	PRERAM_CBFS_CACHE(0x402F4000+16K, 1K)


	ROMSTAGE(0x402F8400, 35K)

	STACK(0x4030b800, 6K)
	SRAM_END(0x40310000)
	DRAM_START(0x80000000)
	RAMSTAGE(0x80000000, 12M)

	/* TODO: Implement MMU support and move TTB to a better location. */
	POSTRAM_CBFS_CACHE(0x81200000, 12M)

#ifdef OMAP_HEADER
	.header : {
		*(.header);
	} : to_load

	/DISCARD/ : {
		*(*)
	}
#endif
}
