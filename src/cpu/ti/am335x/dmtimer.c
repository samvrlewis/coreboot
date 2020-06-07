/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include "dmtimer.h"
#include <device/mmio.h>

void dmtimer_start(int num)
{
	// need to explictly set dmtimer2 to use the right clock source
	write32((void*)(0x44E00500+0x8), 0x1);
	// enable start timer, auto reload, prescale by 64
	//write32((void*)(0x48040000+0x38), 0x3 | (0x5 << 2) | (0x1 << 5));
	write32((void*)(0x48040000+0x38), 0x3);

	return;
}

uint32_t dmtimer_raw_value(int num)
{
	return read32((void*)(0x48040000+0x3c));
}
