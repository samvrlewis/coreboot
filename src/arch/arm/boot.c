/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cache.h>
#include <program_loading.h>

static void run_linux_payload(struct prog *prog)
{
	void (*kernel_entry)(void *sbz, void *sbo, void *dtb);
	void *fdt;

	kernel_entry = prog_entry(prog);
	fdt = prog_entry_arg(prog);
	kernel_entry(0, (void *)~0, fdt);
}

void arch_prog_run(struct prog *prog)
{
	void (*doit)(void *);

	cache_sync_instructions();

	if (ENV_RAMSTAGE && prog_type(prog) == PROG_PAYLOAD) {
		dcache_mmu_disable();
		run_linux_payload(prog);
		return;
	}

	doit = prog_entry(prog);
	doit(prog_entry_arg(prog));
}
