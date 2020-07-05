/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <memlayout.h>

#include <arch/header.ld>

SECTIONS
{
	SRAM_START(0x402f0400)
	BOOTBLOCK(0x402f0400, 20K)
	FMAP_CACHE(0x402f0400+20K, 2K)
	PRERAM_CBFS_CACHE(0x402f0400+20K+2K, 1K)
	TTB(0x402F8000, 16K)
	ROMSTAGE(0x402F8000+16K, 40K)

	/* the cbfs is in here */

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
