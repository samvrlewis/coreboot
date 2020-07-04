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
	write32(&mmc->sysconfig, read32(&mmc->sysconfig) | MMCHS_SD_SYSCONFIG_SOFTRESET);

	while (!(read32(&mmc->sysstatus) & MMCHS_SD_SYSSTATUS_RESETDONE)) { printk(BIOS_DEBUG, "waiting on %d\n", __LINE__); }

	// Set SD capabilities
	write32(&mmc->capa, MMCHS_SD_CAPA_VS18 | MMCHS_SD_CAPA_VS30);

	// one diff - sdbp is turned on later
	write32(&mmc->hctl, MMCHS_SD_HCTL_SDVS_VS30 | MMCHS_SD_HCTL_DTW_1BIT);

	// enable sd bus power
	write32(&mmc->hctl, read32(&mmc->hctl) | MMCHS_SD_HCTL_SDBP);

	while(!(read32(&mmc->hctl) & MMCHS_SD_HCTL_SDBP)) { printk(BIOS_DEBUG, "waiting on %d\n", __LINE__); }

	// set initial clock speed
	// the input clock is at 96MHz so this puts it at 400 KHz
	write32(&mmc->sysctl, read32(&mmc->sysctl) | 240 << 6);

	// enable internal internal clock and clock to card
	write32(&mmc->sysctl, read32(&mmc->sysctl) | MMCHS_SD_SYSCTL_ICE | MMCHS_SD_SYSCTL_CEN);

	//wait for clock to be stable
	while(!(read32(&mmc->sysctl) & MMCHS_SD_SYSCTL_ICS)) { printk(BIOS_DEBUG, "waiting on %d\n", __LINE__); }

	//enable interrupts
	write32(&mmc->ie, 0xffffffffu);

	//clear interrupts
	write32(&mmc->stat, 0xffffffffu);

	// card detection, identification and selection
	write32(&mmc->con, read32(&mmc->con) | MMCHS_SD_CON_INIT);

	write32(&mmc->cmd, 0x00);

	// wait for command complete
	while (!(read32(&mmc->stat) & MMCHS_SD_STAT_CC)) { printk(BIOS_DEBUG, "waiting on %d\n", __LINE__); }

	// clear interrupts
	write32(&mmc->stat, 0xffffffffu);

	// finish initialisation
	write32(&mmc->con, read32(&mmc->con) & ~MMCHS_SD_CON_INIT);

	return 0;
}

static int am335x_send_cmd(struct sd_mmc_ctrlr *ctrlr, struct mmc_command *cmd, struct mmc_data *data)
{
	struct am335x_mmc_host *mmc;
	struct am335x_mmc *reg;

	mmc = container_of(ctrlr, struct am335x_mmc_host, sd_mmc_ctrlr);
	reg = mmc->reg;

	if (read32(&reg->stat) & 0xffffffffu)
	{
		printk(BIOS_WARNING, "SD: Interrupt already raised\n");
		return 1;
	}

	uint32_t transfer_type = 0;

	if (data) {
		if (data->flags & DATA_FLAG_READ) {
			write32(&mmc->reg->ie, MMCHS_SD_IE_BRR_ENABLE_ENABLE);
			write32(&mmc->reg->blk, data->blocksize);
			transfer_type |= MMCHS_SD_CMD_DP_DATA | MMCHS_SD_CMD_DDIR_READ;
		}
		
		if (data->flags & DATA_FLAG_WRITE) {
			printk(BIOS_ERR, "SD: Writes currently not supported\n");
			return 1;
		}
	}

	switch (cmd->resp_type) {
		case CARD_RSP_R1b:
			transfer_type |= MMCHS_SD_CMD_RSP_TYPE_48B_BUSY;
			break;
		case CARD_RSP_R1:
		case CARD_RSP_R3:
			transfer_type |= MMCHS_SD_CMD_RSP_TYPE_48B;
			break;
		case CARD_RSP_R2:
			transfer_type |= MMCHS_SD_CMD_RSP_TYPE_136B;
			break;
		case CARD_RSP_NONE:
			transfer_type |= MMCHS_SD_CMD_RSP_TYPE_NO_RESP;
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
	write32(&reg->cmd, MMCHS_SD_CMD_INDX_CMD(cmd->cmdidx) | transfer_type);

	// Wait for any interrupt
	while (!read32(&reg->stat))
		;

	// Check to ensure that there wasn't any errors
	if (read32(&reg->stat) & MMCHS_SD_STAT_ERRI) {
		printk(BIOS_DEBUG, "Error while reading %08x\n", read32(&reg->stat));
		write32(&reg->stat, MMCHS_SD_STAT_ERROR_MASK);
		return 1;
	}
		
	if (cmd->resp_type == CARD_RSP_R1b) {
		while (!(read32(&reg->stat) & MMCHS_SD_IE_TC_ENABLE_ENABLE)) ;
		write32(&reg->stat, MMCHS_SD_IE_TC_ENABLE_ENABLE);
	}

	write32(&reg->stat, MMCHS_SD_IE_CC_ENABLE_CLEAR);

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
		while(!(read32(&reg->stat) & MMCHS_SD_IE_BRR_ENABLE_ENABLE)) { printk(BIOS_ERR, "SD: Waiting MMCHS_SD_IE_BRR_ENABLE_ENABLE\n"); }

		if (!(read32(&reg->pstate) & MMCHS_SD_PSTATE_BRE_EN)) {
			printk(BIOS_DEBUG, "Can't read from data buffer\n");
			return 1;
		}

		uint32_t* dest32 = (uint32_t*)data->dest;

		for (int count = 0; count < data->blocksize; count+=4)
		{
			*dest32 = read32(&reg->data);
			dest32++; 
		}

		write32(&reg->stat, MMCHS_SD_IE_TC_ENABLE_CLEAR);
		write32(&reg->stat, MMCHS_SD_IE_BRR_ENABLE_CLEAR);
		write32(&mmc->reg->ie, MMCHS_SD_IE_BRR_ENABLE_DISABLE);
	}

	return 0;
}

#define SD_CLK_FREQ 96000000

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

		uint32_t divisor = SD_CLK_FREQ / requested_hz;
		uint32_t actual = SD_CLK_FREQ*divisor;

		if (actual != ctrlr->bus_hz)
		{
			clrbits32(&reg->sysctl, MMCHS_SD_SYSCTL_CEN);
			
			uint32_t new_sysctl = read32(&reg->sysctl);
			new_sysctl &= ~(0x3ff << 6);
			new_sysctl |= (divisor << 6);

			write32(&reg->sysctl, new_sysctl);

			while (!(read32(&reg->sysctl) & MMCHS_SD_SYSCTL_ICS))
				;

			setbits32(&reg->sysctl, MMCHS_SD_SYSCTL_CEN);

			ctrlr->bus_hz = SD_CLK_FREQ/divisor;
			printk(BIOS_DEBUG, "Set SD card freq to %d", ctrlr->bus_hz);
		}
	}
}

static struct storage_media media;
static struct am335x_mmc_host mmc_host;

#include <boot_device.h>
#include <symbols.h>
#include <cbfs.h>


static uint8_t overflow_block[512];

static size_t partial_block_read(uint8_t *dest, uint64_t block, uint32_t offset, uint32_t count)
{
	printk(BIOS_DEBUG, "Reading from block %lld, offset %d, count %d\n", block, offset, count);
	uint64_t blocks_read = storage_block_read(&media, block, 1, &overflow_block);

	if (blocks_read != 1)
	{
		printk(BIOS_DEBUG, "Didn't read 1\n");
		return 0;
	}

	assert((offset + count) <= 512);

	int j=0;
	for (int i=offset; i<(offset+count); i++)
	{
		dest[j++] = overflow_block[i];
	}

	return count;
}


// Needs to support reads that aren't necessarily aligned to the SD card blocks
static ssize_t unleashed_sd_readat(const struct region_device *rdev, void *dest,
					size_t offset, size_t count)
{

	// think I need to find the block that they want based on the offset
	// maybe easier just to preread it out to a buffer.. 
	printk(BIOS_DEBUG, "unleashed read %d %d\n", offset, count);

	uint8_t* buffer = (uint8_t*)dest;

	uint64_t block_start = offset/512;
	uint64_t block_end = (offset+count)/512;
	uint64_t blocks = block_end - block_start + 1;

	// read 20 bytes at offset 1020
	size_t to_read = MIN(512-offset%512, count);
	uint64_t cur_block = block_start;

	size_t bytes_read = partial_block_read(buffer, block_start, offset%512, to_read);
	
	if (blocks == 1)
	{
		return bytes_read;
	}
	
	cur_block++;
	buffer += bytes_read;

	if (blocks > 2)
	{
		// Read all the "whole" blocks between the start and end blocks
		// Read them in one go because then the driver could support reading
		// multiple blocks at a time.
		to_read = blocks - 2;

		printk(BIOS_DEBUG, "Reading from block %lld number of blocks %d\n", cur_block, to_read);
		uint64_t blocks_read = storage_block_read(&media, cur_block, to_read , (void*)buffer);
		
		if (blocks_read != to_read)
		{
			printk(BIOS_DEBUG, "Didn't read all\n");
			return blocks_read*512;
		}
		
		bytes_read += to_read*512;
		cur_block += to_read;
		buffer += bytes_read;
	}

	// read the last block
	bytes_read += partial_block_read(buffer, block_end, 0, count - bytes_read);
	
	return bytes_read;
}

static const struct region_device_ops am335x_sd_ops = {
	.mmap   = mmap_helper_rdev_mmap,
	.munmap = mmap_helper_rdev_munmap,
	.readat = unleashed_sd_readat,
};


static struct mmap_helper_region_device sd_mdev =
	MMAP_HELPER_REGION_INIT(&am335x_sd_ops, 0, 10200*1024);

const struct region_device *boot_device_ro(void)
{
	return &sd_mdev.rdev;
}

static void boot_from_sd(void)
{
	uint64_t blocks_read = storage_block_read(&media, 0, 19593, (void*)(0x82000000));
	blocks_read = storage_block_read(&media, 19593, 185, (void*)(0x88000000));

	uint8_t* fdt = (uint8_t*)(0x88000000);
	for (int i=0; i < 20; i++)
	{
			printk(BIOS_DEBUG, "%x\n", fdt[i]);
	}

	dcache_mmu_disable();
	
	printk(BIOS_DEBUG, "turned it off\n");
	void (*kernel_entry)(void *, void*, void*);
	kernel_entry = (void*)(0x82000000);
	kernel_entry(0, (void*)~0, (void*)(0x88000000));
}

void init_sd(void)
{
	
    struct am335x_mmc *mmc = (void*)0x48060000;

	mmc_host.reg = mmc;

	struct sd_mmc_ctrlr *mmc_ctrlr = &mmc_host.sd_mmc_ctrlr;

	am335x_mmc_init(mmc);

	memset(mmc_ctrlr, 0, sizeof(mmc_ctrlr));
	mmc_ctrlr->send_cmd = &am335x_send_cmd;
	mmc_ctrlr->set_ios = &set_ios;

	mmc_ctrlr->voltages = MMC_VDD_30_31;
	mmc_ctrlr->b_max = 1; //only support 1 block at a time
	mmc_ctrlr->bus_width = 1;
	mmc_ctrlr->f_max = 48000000;
	mmc_ctrlr->f_min =  400000;
	mmc_ctrlr->bus_hz = 400000;

	media.ctrlr = mmc_ctrlr;

	printk(BIOS_DEBUG, "pre storage\n");
	storage_setup_media(&media, mmc_ctrlr);
	printk(BIOS_DEBUG, "post storage\n");
	storage_display_setup(&media);
	printk(BIOS_DEBUG, "reading blocks\n");
		
	if (0)
	{
		boot_from_sd();
	}


	//storage_block_read(&media, 0, (10200*1024)/512, &buffer);

	//printk(BIOS_DEBUG, "read %llu blocks\n", blocks_read);
	mmap_helper_device_init(&sd_mdev,
			_cbfs_cache, REGION_SIZE(cbfs_cache));



	return;



	//storage_block_read(&media, 0, 1, &buffer);

	//storage_block_read(&media, 0, 1, &buffer);

	

	//	printk(BIOS_DEBUG, "post storage\n");
}