/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include "dmtimer.h"
#include <device/mmio.h>

void dmtimer_start(int num)
{
	write32((void*)(0x48040000+0x38), 0x3);
	return;
}

uint32_t dmtimer_raw_value(int num)
{
	return read32((void*)(0x48040000+0x3c));
}
