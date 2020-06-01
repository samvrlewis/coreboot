/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <program_loading.h>
#include <console/console.h>
#include <cpu/ti/am335x/ddr_init_x.h>

#define uart_putf(X...) printk(BIOS_INFO, "romstage: " X)

void main(void)
{
	console_init();
	printk(BIOS_INFO, "Hello from romstage.\n");

	config_ddr_x();


 	{
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

	run_ramstage();
}
