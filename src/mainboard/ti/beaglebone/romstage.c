/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <program_loading.h>
#include <console/console.h>
#include <cbmem.h>
#include <cpu/ti/am335x/ddr_init_x.h>
#include <cpu/ti/am335x/ddr_init.h>

#include <lib.h>
#include <symbols.h>

#define uart_putf(X...) printk(BIOS_INFO, "romstage: " X)


/* Micron MT41K256M16HA-125E */
#define MT41K256M16HA125E_EMIF_READ_LATENCY	0x100007
#define MT41K256M16HA125E_EMIF_TIM1		0x0AAAD4DB
#define MT41K256M16HA125E_EMIF_TIM2		0x266B7FDA
#define MT41K256M16HA125E_EMIF_TIM3		0x501F867F
#define MT41K256M16HA125E_EMIF_SDCFG		0x61C05332
#define MT41K256M16HA125E_EMIF_SDREF		0xC30
#define MT41K256M16HA125E_ZQ_CFG		0x50074BE4
#define MT41K256M16HA125E_RATIO			0x80
#define MT41K256M16HA125E_INVERT_CLKOUT		0x0
#define MT41K256M16HA125E_RD_DQS		0x38
#define MT41K256M16HA125E_WR_DQS		0x44
#define MT41K256M16HA125E_PHY_WR_DATA		0x7D
#define MT41K256M16HA125E_PHY_FIFO_WE		0x94
#define MT41K256M16HA125E_IOCTRL_VALUE		0x18B

#define EMIF_OCP_CONFIG_BEAGLEBONE_BLACK       0x00141414


const struct ctrl_ioregs ioregs_bonelt = {
	.cm0ioctl		= MT41K256M16HA125E_IOCTRL_VALUE,
	.cm1ioctl		= MT41K256M16HA125E_IOCTRL_VALUE,
	.cm2ioctl		= MT41K256M16HA125E_IOCTRL_VALUE,
	.dt0ioctl		= MT41K256M16HA125E_IOCTRL_VALUE,
	.dt1ioctl		= MT41K256M16HA125E_IOCTRL_VALUE,
};

static const struct ddr_data ddr3_beagleblack_data = {
	.datardsratio0 = MT41K256M16HA125E_RD_DQS,
	.datawdsratio0 = MT41K256M16HA125E_WR_DQS,
	.datafwsratio0 = MT41K256M16HA125E_PHY_FIFO_WE,
	.datawrsratio0 = MT41K256M16HA125E_PHY_WR_DATA,
};

static const struct cmd_control ddr3_beagleblack_cmd_ctrl_data = {
	.cmd0csratio = MT41K256M16HA125E_RATIO,
	.cmd0iclkout = MT41K256M16HA125E_INVERT_CLKOUT,

	.cmd1csratio = MT41K256M16HA125E_RATIO,
	.cmd1iclkout = MT41K256M16HA125E_INVERT_CLKOUT,

	.cmd2csratio = MT41K256M16HA125E_RATIO,
	.cmd2iclkout = MT41K256M16HA125E_INVERT_CLKOUT,
};

static struct emif_regs ddr3_beagleblack_emif_reg_data = {
	.sdram_config = MT41K256M16HA125E_EMIF_SDCFG,
	.ref_ctrl = MT41K256M16HA125E_EMIF_SDREF,
	.sdram_tim1 = MT41K256M16HA125E_EMIF_TIM1,
	.sdram_tim2 = MT41K256M16HA125E_EMIF_TIM2,
	.sdram_tim3 = MT41K256M16HA125E_EMIF_TIM3,
	.ocp_config = EMIF_OCP_CONFIG_BEAGLEBONE_BLACK,
	.zq_config = MT41K256M16HA125E_ZQ_CFG,
	.emif_ddr_phy_ctlr_1 = MT41K256M16HA125E_EMIF_READ_LATENCY,
};

void main(void)
{
	console_init();
	printk(BIOS_INFO, "Hello from romstage.\n");
//config_ddr_x();
			config_ddr(400, &ioregs_bonelt,
			   &ddr3_beagleblack_data,
			   &ddr3_beagleblack_cmd_ctrl_data,
			   &ddr3_beagleblack_emif_reg_data, 0);

			   


	printk(BIOS_INFO, "RAM INIT Done\n");

 	if (0) {
	    #include <stdint.h>
	    uint32_t	*p = (uint32_t*)0x80000000;
	  	uint32_t	*endp = (uint32_t*)(0x80000000+512*1024*1024);
//	  	uint32_t	*endp = (uint32_t*)(0x80000000+64*1024*1024);
	  	int			i=0;
	  	
	  	uart_putf("start DDR3 ram test, from 0x%p to 0x%p\n",p,endp);
	  	
	  	uart_putf("    writing...\n");
	  	while ( p < endp )
	  	{
		  	*p = (uint32_t)i;
		  	p++;
		  	i++;
	  	}
	  	
	  	i=0;
	  	p = (uint32_t*)0x80000000;

	  	uart_putf("    checking...\n");
	  	while ( p < endp )
	  	{
		  	if ( *p != (uint32_t)i )
		  	{
			  	uart_putf("mem check error\n");
			  	//uart_putf("    address : 0x%x\n",p);
			  	//uart_putf("    expected: 0x%x\n",i);
			  	//uart_putf("    actual  : 0x%x\n",*p);
		  		p = endp + 8;
	  		}
		  	else
		  	{
		  		p++;
		  		i++;
  			}
	  	}
	  	
	  	if ( p != (endp + 8) )
	  		uart_putf("DDR3 ram test OK\n");
	  	else
	  		uart_putf("DDR3 ram test FAIL\n");
    }

	printk(BIOS_DEBUG, "CONFIG_ROM_SIZE %d\n", CONFIG_ROM_SIZE);

	cbmem_initialize_empty();
	ram_check(0x80000000, 0x80500000);

	run_ramstage();
}
