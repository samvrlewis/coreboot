#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <console/console.h>
#include <commonlib/storage/sd_mmc.h>
#include <commonlib/sd_mmc_ctrlr.h>
#include <device/mmio.h>
#include <delay.h>
#include <assert.h>
#include "sd.h"

static int am335x_mmc_init(struct am335x_mmc *mmc)
{
	// Follows the initialisiation guide from the AM335X technical reference manual
	setbits32(&mmc->sysconfig, SYSCONFIG_SOFTRESET);

	while (!(read32(&mmc->sysstatus) & SYSSTATUS_RESETDONE)) ;

	// Set SD capabilities
	setbits32(&mmc->capa, CAPA_VS30);

	setbits32(&mmc->hctl, HCTL_SDVS_VS30 | HCTL_DTW_1BIT);

	// enable sd bus power
	setbits32(&mmc->hctl, HCTL_SDBP);

	while(!(read32(&mmc->hctl) & HCTL_SDBP)) ;

	// set initial clock speed
	// the input clock is at 96MHz so this puts it at 400 KHz
	// give a generous timeout
	write32(&mmc->sysctl, read32(&mmc->sysctl) | 240 << 6 | SYSCTL_DTO_15);

	// enable internal internal clock and clock to card
	setbits32(&mmc->sysctl, SYSCTL_ICE | SYSCTL_CEN);

	//wait for clock to be stable
	while(!(read32(&mmc->sysctl) & SYSCTL_ICS)) ;

	// enable command complete interrupt only
	write32(&mmc->ie, IE_ERRORS | IE_TC | IE_CC);

	//clear interrupts
	write32(&mmc->stat, 0xffffffffu);

	// card detection, identification and selection
	setbits32(&mmc->con, CON_INIT);

	write32(&mmc->cmd, 0x00);

	// wait for command complete
	while (!(read32(&mmc->stat) & STAT_CC)) ;

	// clear interrupts
	write32(&mmc->stat, 0xffffffffu);

	// finish initialisation
	clrbits32(&mmc->con, CON_INIT);

	return 0;
}

static int am335x_send_cmd(struct sd_mmc_ctrlr *ctrlr, struct mmc_command *cmd, struct mmc_data *data)
{
	struct am335x_mmc_host *mmc;
	struct am335x_mmc *reg;

	mmc = container_of(ctrlr, struct am335x_mmc_host, sd_mmc_ctrlr);
	reg = mmc->reg;

	if (read32(&reg->stat))
	{
		printk(BIOS_WARNING, "SD: Interrupt already raised\n");
		return 1;
	}

	uint32_t transfer_type = 0;

	if (data) {
		if (data->flags & DATA_FLAG_READ) {
			setbits32(&mmc->reg->ie, IE_BRR);
			write32(&mmc->reg->blk, data->blocksize);
			transfer_type |= CMD_DP_DATA | CMD_DDIR_READ;
		}
		
		if (data->flags & DATA_FLAG_WRITE) {
			printk(BIOS_ERR, "SD: Writes currently not supported\n");
			return 1;
		}
	}

	switch (cmd->resp_type) {
		case CARD_RSP_R1b:
			transfer_type |= CMD_RSP_TYPE_48B_BUSY;
			break;
		case CARD_RSP_R1:
		case CARD_RSP_R3:
			transfer_type |= CMD_RSP_TYPE_48B;
			break;
		case CARD_RSP_R2:
			transfer_type |= CMD_RSP_TYPE_136B;
			break;
		case CARD_RSP_NONE:
			transfer_type |= CMD_RSP_TYPE_NO_RESP;
			break;
		default:
			printk(BIOS_ERR, "SD: Unknown response type\n");
			return 1;
	}

	if (cmd->cmdidx == MMC_CMD_SET_BLOCKLEN) {
		//write32(&mmc->reg->blk, cmd->cmdarg);
		return 0;
	}

	write32(&reg->arg, cmd->cmdarg);
	write32(&reg->cmd, CMD_INDEX(cmd->cmdidx) | transfer_type);

	// Wait for any interrupt
	while (!read32(&reg->stat))
		;
		
	// Check to ensure that there wasn't any errors
	if (read32(&reg->stat) & STAT_ERRI) {
		printk(BIOS_WARNING, "Error while reading %08x\n", read32(&reg->stat));

		// clear the errors
		write32(&reg->stat, STAT_ERROR_MASK);
		return 1;
	}
		
	if (cmd->resp_type == CARD_RSP_R1b) {
		while (!(read32(&reg->stat) & IE_TC)) ;
		write32(&reg->stat, IE_TC);
	}

	write32(&reg->stat, IE_CC);

		/* copy response into cmd->resp	 */
	switch (cmd->resp_type) {
		case CARD_RSP_R1:
		case CARD_RSP_R1b:
		case CARD_RSP_R3:
			cmd->response[0] = read32(&reg->rsp10);
			break;
		case CARD_RSP_R2:
			cmd->response[3] = read32(&reg->rsp10);
			cmd->response[2] = read32(&reg->rsp32);
			cmd->response[1] = read32(&reg->rsp54);
			cmd->response[0] = read32(&reg->rsp76);
			break;
		case CARD_RSP_NONE:
			break;
	}

	if (data != NULL && data->flags & DATA_FLAG_READ) {
		while(!(read32(&reg->stat) & IE_BRR)) ;

		uint32_t* dest32 = (uint32_t*)data->dest;

		for (int count = 0; count < data->blocksize; count+=4)
		{
			*dest32 = read32(&reg->data);
			dest32++; 
		}

		write32(&reg->stat, IE_TC);
		write32(&reg->stat, IE_BRR);
		clrbits32(&reg->ie, IE_BRR);
	}

	return 0;
}

static void set_ios(struct sd_mmc_ctrlr *ctrlr)
{
	struct am335x_mmc_host *mmc;
	struct am335x_mmc *reg;

	mmc = container_of(ctrlr, struct am335x_mmc_host, sd_mmc_ctrlr);
	reg = mmc->reg;

	if (ctrlr->request_hz != ctrlr->bus_hz)
	{
		uint32_t requested_hz = ctrlr->request_hz;

		/* Compute the divisor for the new clock frequency */
		requested_hz = MIN(requested_hz, ctrlr->f_min);
		requested_hz = MAX(requested_hz, ctrlr->f_max);

		uint32_t divisor = mmc->sd_clock_hz / requested_hz;
		uint32_t actual = mmc->sd_clock_hz*divisor;

		if (actual != ctrlr->bus_hz)
		{
			clrbits32(&reg->sysctl, SYSCTL_CEN);
			
			uint32_t new_sysctl = read32(&reg->sysctl);
			new_sysctl &= ~(0x3ff << 6);
			new_sysctl |= ((divisor & 0x3ff) << 6);

			write32(&reg->sysctl, new_sysctl);

			// wait for clock stability
			while (!(read32(&reg->sysctl) & SYSCTL_ICS))
				;

			setbits32(&reg->sysctl, SYSCTL_CEN);

			ctrlr->bus_hz = mmc->sd_clock_hz/divisor;
			printk(BIOS_DEBUG, "Set SD card freq to %d", ctrlr->bus_hz);
		}
	}
}

int am335x_sd_init_storage(struct am335x_mmc_host * mmc_host)
{
	int err = 0;

	struct sd_mmc_ctrlr *mmc_ctrlr = &mmc_host->sd_mmc_ctrlr;
	memset(mmc_ctrlr, 0, sizeof(mmc_ctrlr));


	err = am335x_mmc_init(mmc_host->reg);
	if (err != 0)
	{
		printk(BIOS_ERR, "ERROR: Initialising AM335X SD failed.\n");
		return err;
	}

	mmc_ctrlr->send_cmd = &am335x_send_cmd;
	mmc_ctrlr->set_ios = &set_ios;

	mmc_ctrlr->voltages = MMC_VDD_30_31;
	mmc_ctrlr->b_max = 1;
	mmc_ctrlr->bus_width = 1;
	mmc_ctrlr->f_max = 48000000;
	mmc_ctrlr->f_min =  400000;
	mmc_ctrlr->bus_hz = 400000;

	return 0;
}