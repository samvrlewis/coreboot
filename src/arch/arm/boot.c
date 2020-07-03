/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <arch/cache.h>
#include <program_loading.h>
#include <console/console.h>


void arch_prog_run(struct prog *prog)
{
	void (*doit)(void *);

	cache_sync_instructions();

	doit = prog_entry(prog);

	if (ENV_RAMSTAGE) {
		uint8_t* fdt = (uint8_t*)prog_entry_arg(prog);
		for (int i=0; i < 20; i++)
		{
				printk(BIOS_DEBUG, "%x\n", fdt[i]);
		}

		printk(BIOS_DEBUG, "----------------\n");
		uint8_t* kern = (uint8_t*)prog_entry(prog);
		for (int i=0; i < 20; i++)
		{
				printk(BIOS_DEBUG, "%x\n", kern[i]);
		}

		dcache_mmu_disable();
		printk(BIOS_DEBUG, "turned it off\n");
		void (*kernel_entry)(void *, void*, void*);
		kernel_entry = (void*)prog_entry(prog);
		kernel_entry(0, (void*)~0, (void*)(prog_entry_arg(prog)));
	} else {
		doit(prog_entry_arg(prog));
	}
}
