/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <boot_device.h>
#include <symbols.h>

/* FIXME: No idea how big the internal SRAM actually is. */
static const struct mem_region_device boot_dev =
	MEM_REGION_DEV_RO_INIT((void *)0x402f0400, 111616);

const struct region_device *boot_device_ro(void)
{
	return &boot_dev.rdev;
}
