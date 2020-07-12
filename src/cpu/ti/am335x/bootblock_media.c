/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <boot_device.h>
#include <symbols.h>
#include "sd.h"
#include <console/console.h>
#include <assert.h>
#include <commonlib/storage/sd_mmc.h>
#include <cbmem.h>
#include "header.h"

#define SD_BLOCK_SIZE 512

static struct am335x_mmc_host sd_host;
static struct storage_media media;

static size_t partial_block_read(uint8_t *dest, uint64_t block, uint32_t offset, uint32_t count)
{
	static uint8_t overflow_block[SD_BLOCK_SIZE];

	uint64_t blocks_read = storage_block_read(&media, block, 1, &overflow_block);

	if (blocks_read != 1)
	{
		printk(BIOS_ERR, "Read different blocks than expecting: %llu\n", blocks_read);
		return 0;
	}

	assert((offset + count) <= SD_BLOCK_SIZE);

	int j=0;
	for (int i=offset; i<(offset+count); i++)
	{
		dest[j++] = overflow_block[i];
	}

	return count;
}


// Needs to support reads that aren't necessarily aligned to the SD card blocks
static ssize_t sd_readat(const struct region_device *rdev, void *dest,
					size_t offset, size_t count)
{
	uint8_t* buffer = (uint8_t*)dest;

	uint64_t block_start = offset/SD_BLOCK_SIZE;
	uint64_t block_end = (offset+count)/SD_BLOCK_SIZE;
	uint64_t blocks = block_end - block_start + 1;

	// how much of the first block should we read
	uint32_t first_block_offset = offset%SD_BLOCK_SIZE;
	size_t first_block_to_read = MIN(SD_BLOCK_SIZE - first_block_offset, count);

	size_t bytes_read = partial_block_read(buffer, block_start, first_block_offset, first_block_to_read);
	
	if (blocks == 1)
	{
		return bytes_read;
	}
	
	buffer += bytes_read;

	if (blocks > 2)
	{
		// Read all the "whole" blocks between the start and end blocks
		// Read them in one go because then the driver could support reading
		// multiple blocks at a time.
		uint64_t to_read = blocks - 2;

		//printk(BIOS_DEBUG, "Reading from block %lld number of blocks %d\n", cur_block, to_read);
		//printk(BIOS_DEBUG, "Writing to %p\n", buffer);
		uint64_t blocks_read = storage_block_read(&media, block_start+1, to_read, (void*)buffer);

		if (blocks_read != to_read)
		{
			printk(BIOS_ERR, "Expecting to read %llu blocks but only read %llu\n", to_read, blocks_read);
			return blocks_read*SD_BLOCK_SIZE;
		}
		
		buffer += to_read*SD_BLOCK_SIZE;
		bytes_read += to_read*SD_BLOCK_SIZE;
	}

	// read the last block, which might not be aligned on a SD block
	bytes_read += partial_block_read(buffer, block_end, 0, count - bytes_read);
	
	return bytes_read;
}

static const struct region_device_ops am335x_sd_ops = {
	.mmap   = mmap_helper_rdev_mmap,
	.munmap = mmap_helper_rdev_munmap,
	.readat = sd_readat,
};

extern struct omap_image_headers headers;

static struct mmap_helper_region_device sd_mdev =
	MMAP_HELPER_REGION_INIT(&am335x_sd_ops, sizeof(headers), 22200*1024);

static void switch_to_postram_cache(int unused)
{
	// todo: figure out if this is necessary
	boot_device_init();
	if (_preram_cbfs_cache != _postram_cbfs_cache)
	{
		mmap_helper_device_init(&sd_mdev, _postram_cbfs_cache,
					REGION_SIZE(postram_cbfs_cache));
	}
}
ROMSTAGE_CBMEM_INIT_HOOK(switch_to_postram_cache);

static bool mmc_init_done = false;

void boot_device_init(void)
{
	if (mmc_init_done)
	{
		return;
	}

	printk(BIOS_DEBUG, "Initing this bizzines\n");
	sd_host.sd_clock_hz = 96000000;
	sd_host.reg = (void*)0x48060000;
	am335x_sd_init_storage(&sd_host);
	storage_setup_media(&media, &sd_host.sd_mmc_ctrlr);
	storage_display_setup(&media);

	// todo: this is a bit ugly but the romstage needs a bigger cache
	// in order to load the ramstage. can this been done with hte postram cache swap above?
	if (ENV_BOOTBLOCK)
	{
		mmap_helper_device_init(&sd_mdev,
			_cbfs_cache, REGION_SIZE(cbfs_cache));
	} else {
		mmap_helper_device_init(&sd_mdev, _postram_cbfs_cache,
				REGION_SIZE(postram_cbfs_cache));
	}

	mmc_init_done = true;
}

const struct region_device *boot_device_ro(void)
{
	return &sd_mdev.rdev;
}