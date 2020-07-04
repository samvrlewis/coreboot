/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <boot_device.h>
#include <symbols.h>

// Needs to support reads that aren't necessarily aligned to the SD card blocks
static ssize_t unleashed_sd_readat(const struct region_device *rdev, void *dest,
					size_t offset, size_t count)
{
	uint8_t *mem = (uint8_t*)0x402f0400;
	uint8_t *buffer = (uint8_t*)dest;
	
	int j=0;


	for (int i=offset; i<offset+count; i++)
	{
		buffer[j++] = mem[i];
	}

	return count;
}

static const struct region_device_ops am335x_sd_ops = {
	.mmap   = mmap_helper_rdev_mmap,
	.munmap = mmap_helper_rdev_munmap,
	.readat = unleashed_sd_readat,
};


static struct mmap_helper_region_device sd_mdev =
	MMAP_HELPER_REGION_INIT(&am335x_sd_ops, 0, 111616);


static int init = 0;

const struct region_device *boot_device_ro(void)
{
	if (!init)
	{
			mmap_helper_device_init(&sd_mdev,
			_cbfs_cache, REGION_SIZE(cbfs_cache));

	}

	return &sd_mdev.rdev;
}