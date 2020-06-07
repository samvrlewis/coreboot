/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <stdint.h>
#include <delay.h>
#include <timer.h>

#include "dmtimer.h"

static struct monotonic_counter {
	int initialized;
	struct mono_time time;
	uint64_t last_value;
} mono_counter;

// OSC Hz= clocks in a second
static const uint32_t clocks_per_usec = OSC_HZ/1000000;

void timer_monotonic_get(struct mono_time *mt)
{
	if (!mono_counter.initialized) {
		dmtimer_start(1);
		mono_counter.initialized = 1;
	}

	mono_time_set_usecs(mt, dmtimer_raw_value(1) / clocks_per_usec);
}
