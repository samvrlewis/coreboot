/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <boot_device.h>
#include <symbols.h>
#include "sd.h"
#include <console/console.h>
#include <assert.h>
#include <commonlib/storage/sd_mmc.h>

static struct storage_media *media;
static volatile int sd = 1;

static uint8_t overflow_block[512];

static size_t partial_block_read(uint8_t *dest, uint64_t block, uint32_t offset, uint32_t count)
{
	printk(BIOS_DEBUG, "Reading from block %lld, offset %d, count %d\n", block, offset, count);
	printk(BIOS_DEBUG, "Writing to %p\n", dest);
	uint64_t blocks_read = storage_block_read(media, block, 1, &overflow_block);

	if (blocks_read != 1)
	{
		printk(BIOS_DEBUG, "Didn't read 1\n");
		return 0;
	}

	assert((offset + count) <= 512);

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

	// think I need to find the block that they want based on the offset
	// maybe easier just to preread it out to a buffer.. 
	printk(BIOS_DEBUG, "unleashed read %d %d\n", offset, count);

	uint8_t* buffer = (uint8_t*)dest;

	uint64_t block_start = offset/512;
	uint64_t block_end = (offset+count)/512;
	uint64_t blocks = block_end - block_start + 1;

	// read 20 bytes at offset 1020
	size_t to_read = MIN(512-offset%512, count);
	uint64_t cur_block = block_start;

	size_t bytes_read = partial_block_read(buffer, block_start, offset%512, to_read);
	
	if (blocks == 1)
	{
		return bytes_read;
	}
	
	cur_block++;
	buffer += bytes_read;

	if (blocks > 2)
	{
		// Read all the "whole" blocks between the start and end blocks
		// Read them in one go because then the driver could support reading
		// multiple blocks at a time.
		to_read = blocks - 2;

		printk(BIOS_DEBUG, "Reading from block %lld number of blocks %d\n", cur_block, to_read);
		printk(BIOS_DEBUG, "Writing to %p\n", buffer);
		uint64_t blocks_read = storage_block_read(media, cur_block, to_read , (void*)buffer);
		
		if (blocks_read != to_read)
		{
			printk(BIOS_DEBUG, "Didn't read all\n");
			return blocks_read*512;
		}
		
		bytes_read = to_read*512;
		cur_block += to_read;
		buffer += bytes_read;
	}

	// read the last block
	bytes_read += partial_block_read(buffer, block_end, 0, count - bytes_read);
	
	return bytes_read;
}

// Needs to support reads that aren't necessarily aligned to the SD card blocks
static ssize_t generic_readat(const struct region_device *rdev, void *dest,
					size_t offset, size_t count)
{
	uint8_t *mem = (uint8_t*)0x402f0400;
	uint8_t *buffer = (uint8_t*)dest;

	printk(BIOS_DEBUG, "generic read %d %d\n", offset, count);
	
	if (sd)
	{
		count = sd_readat(rdev, dest, offset, count);
		int j=0;
		for (int i=offset; i<offset+count; i++)
		{
			if (buffer[j++] != mem[i])
			{
				printk(BIOS_DEBUG, "MISMATCH SD:%x MEM:%x IND:%d ADDR:%p DESTADDR:%p\n", buffer[j-1], mem[i], j-1, &mem[i], &buffer[j-1]);
			}
		}
	} else {
		int j=0;
		for (int i=offset; i<offset+count; i++)
		{
			buffer[j++] = mem[i];
		}
	}

	return count;
}

static const struct region_device_ops am335x_sd_ops = {
	.mmap   = mmap_helper_rdev_mmap,
	.munmap = mmap_helper_rdev_munmap,
	.readat = generic_readat,
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

		if(sd)
		{
			media = init_sd();
		}
		
		init = 1;

	}

	return &sd_mdev.rdev;
}