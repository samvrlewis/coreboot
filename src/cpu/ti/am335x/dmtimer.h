/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#ifndef __CPU_TI_AM335X_DMTIMER_H__
#define __CPU_TI_AM335X_DMTIMER_H__

#include <stdint.h>

#define OSC_HZ (25000000)

void dmtimer_start(int num);
uint32_t dmtimer_raw_value(int num);

#endif
