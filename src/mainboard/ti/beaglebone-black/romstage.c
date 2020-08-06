/* SPDX-License-Identifier: GPL-2.0-only */

#include <program_loading.h>
#include <console/console.h>
#include <cbmem.h>

#include <soc/ti/am335x/sdram.h>
#include "ddr3.h"

void main(void)
{
	console_init();
	printk(BIOS_INFO, "Hello from romstage.\n");

	config_ddr(400, &ioregs_bonelt, &ddr3_beagleblack_data, &ddr3_beagleblack_cmd_ctrl_data,
		   &ddr3_beagleblack_emif_reg_data, 0);

	cbmem_initialize_empty();

	run_ramstage();
}
