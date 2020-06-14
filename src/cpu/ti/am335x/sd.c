#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <console/console.h>
#include <commonlib/storage/sd_mmc.h>
#include <commonlib/sd_mmc_ctrlr.h>
#include <device/mmio.h>

#include "sd.h"

#define uart_putf(X...) printk(BIOS_INFO, "romstage: " X)

/**
 * Write a uint32_t value to a memory address
 */
static inline void write32x(uint32_t address, uint32_t value)
{
	REG(address)= value;
}

/**
 * Read an uint32_t from a memory address
 */
static inline uint32_t read32x(uint32_t address)
{
	return REG(address);
}

/**
 * Set a 32 bits value depending on a mask
 */
static inline void set32(uint32_t address, uint32_t mask, uint32_t value)
{
	uint32_t val;
	val= read32x(address);
	val&= ~(mask); /* clear the bits */
	val|= (value & mask); /* apply the value using the mask */
	write32x(address, val);
}

static inline void set32_with_mask(void *addr, uint32_t mask, uint32_t value)
{
    uint32_t val;
	val= read32(addr);
	val&= ~(mask); /* clear the bits */
	val|= (value & mask); /* apply the value using the mask */
	write32(addr, val);
}

static int am335x_mmc_init(struct am335x_mmc *mmc)
{
    int counter=0;
	/*
	 * Soft reset of the controller
	 */
	/* Write 1 to sysconfig[0] to trigger a reset*/
    set32_with_mask(&mmc->sysconfig, MMCHS_SD_SYSCONFIG_SOFTRESET, MMCHS_SD_SYSCONFIG_SOFTRESET);

	/* read sysstatus to know it's done */
	while (!(read32(&mmc->sysstatus)
			& MMCHS_SD_SYSSTATUS_RESETDONE))
            ;

    set32_with_mask(&mmc->sysctl, MMCHS_SD_SYSCTL_SRA, MMCHS_SD_SYSCTL_SRA);

	while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL)
			& MMCHS_SD_SYSCTL_SRA)) 
        ;
    
    /*
	 * Set SD default capabilities
	 */
    write32(&mmc->hctl, MMCHS_SD_HCTL_SDVS_VS30 | MMCHS_SD_HCTL_SDBP_OFF | MMCHS_SD_HCTL_DTW_1BIT);
	set32_with_mask(&mmc->capa, MMCHS_SD_CAPA_VS_MASK, MMCHS_SD_CAPA_VS18 | MMCHS_SD_CAPA_VS30);



	/*
	 * MMC host and bus configuration
	 */

	/* Configure data and command transfer (1 bit mode)*/
	set32(MMCHS0_REG_BASE + MMCHS_SD_CON, MMCHS_SD_CON_DW8,
			MMCHS_SD_CON_DW8_1BIT);


	/* Configure card voltage  */
	set32(MMCHS0_REG_BASE + MMCHS_SD_HCTL, MMCHS_SD_HCTL_SDVS,
			MMCHS_SD_HCTL_SDVS_VS30 /* Configure 3.0 volt */
			);

	/* Power on the host controller and wait for the  MMCHS_SD_HCTL_SDBP_POWER_ON to be set */
	set32(MMCHS0_REG_BASE + MMCHS_SD_HCTL, MMCHS_SD_HCTL_SDBP,
			MMCHS_SD_HCTL_SDBP_ON);

	while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_HCTL) & MMCHS_SD_HCTL_SDBP)
			!= MMCHS_SD_HCTL_SDBP_ON) {
		counter++;
	}

	/* Enable internal clock and clock to the card*/
	set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, MMCHS_SD_SYSCTL_ICE,
			MMCHS_SD_SYSCTL_ICE_EN);

	//@TODO Fix external clock enable , this one is very slow
	set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, MMCHS_SD_SYSCTL_CLKD,
			(240 << 6));
	set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, MMCHS_SD_SYSCTL_CEN,
			MMCHS_SD_SYSCTL_CEN_EN);
	while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL) & MMCHS_SD_SYSCTL_ICS)
			!= MMCHS_SD_SYSCTL_ICS_STABLE)
		;

	/*
	 * See spruh73e page 3576  Card Detection, Identification, and Selection
	 */

	/* enable command interrupt */
	set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_CC_ENABLE,
			MMCHS_SD_IE_CC_ENABLE_ENABLE);
	/* enable transfer complete interrupt */
	set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_TC_ENABLE,
			MMCHS_SD_IE_TC_ENABLE_ENABLE);

	/* enable error interrupts */
	/* NOTE: We are currently skipping the BADA interrupt it does get raised for unknown reasons */
	set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_ERROR_MASK, 0x0fffffffu);
	//set32(MMCHS0_REG_BASE + MMCHS_SD_IE,MMCHS_SD_IE_ERROR_MASK, 0xffffffffu);

	/* clean the error interrupts */
	set32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_STAT_ERROR_MASK,
			0xffffffffu); // clear errors
	//set32(MMCHS0_REG_BASE + MMCHS_SD_STAT,MMCHS_SD_STAT_ERROR_MASK, 0xffffffffu);// clear errors

	/* send a init signal to the host controller. This does not actually
	 * send a command to a card manner
	 */
	set32(MMCHS0_REG_BASE + MMCHS_SD_CON, MMCHS_SD_CON_INIT,
			MMCHS_SD_CON_INIT_INIT);
	write32x(MMCHS0_REG_BASE + MMCHS_SD_CMD, 0x00); /* command 0 , type other commands , not response etc) */

	while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT) & MMCHS_SD_STAT_CC)
			!= MMCHS_SD_STAT_CC_RAISED) {
		if (read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0x8000) {
			printk(BIOS_DEBUG,"%s, error stat  %x\n", __FUNCTION__,
					read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT));
			return 1;
		}
		counter++;
	}

	/* clear the cc interrupt status */
	set32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_CC_ENABLE,
			MMCHS_SD_IE_CC_ENABLE_ENABLE);

	/*
	 * Set Set SD_CON[1] INIT bit to 0x0 to end the initialization sequence
	 */
	set32(MMCHS0_REG_BASE + MMCHS_SD_CON, MMCHS_SD_CON_INIT,
			MMCHS_SD_CON_INIT_NOINIT);
	return 0;
}


static int send_cmd_base(uint32_t command, uint32_t arg, uint32_t resp_type)
{
	int count= 0;

	uart_putf("send cmdbase %d %d %d\n", command, arg, resp_type);
	/* Read current interrupt status and fail it an interrupt is already asserted */
	if ((read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0xffffu)) {
		printk(BIOS_DEBUG,"%s, interrupt already raised stat  %08x\n", __FUNCTION__,
				read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT));
		return 1;
	}

	/* Set arguments */
	write32x(MMCHS0_REG_BASE + MMCHS_SD_ARG, arg);
	/* Set command */
	set32(MMCHS0_REG_BASE + MMCHS_SD_CMD, MMCHS_SD_CMD_MASK, command);

	/* Wait for completion */
	while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0xffffu) == 0x0) {
		count++;
	}

	if (read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0x8000) {
		printk(BIOS_DEBUG,"%s, error stat  %08x\n", __FUNCTION__,
				read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT));
		set32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_STAT_ERROR_MASK,
				0xffffffffu);	// clear errors
		// We currently only support 2.0, not responding to
		return 1;
	}

	if (resp_type == CARD_RSP_R1b) {
		/*
		 * Command with busy repsonse *CAN* also set the TC bit if they exit busy
		 */
		while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT)
				& MMCHS_SD_IE_TC_ENABLE_ENABLE) == 0) {
			count++;
		}
		write32x(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_TC_ENABLE_CLEAR);
	}

	/* clear the cc status */
	write32x(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_CC_ENABLE_CLEAR);
	return 0;
}

static int send_cmd(struct sd_mmc_ctrlr *ctrlr, struct mmc_command *cmd, struct mmc_data *data)
{

    uint32_t count;
	uint32_t value;
	uart_putf("send cmd %d %d %d\n", cmd->cmdidx, cmd->cmdarg, cmd->resp_type);

	int ret = 0;

	if (cmd->cmdidx == 17)
	{
		set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_BRR_ENABLE,
			MMCHS_SD_IE_BRR_ENABLE_ENABLE);

		set32(MMCHS0_REG_BASE + MMCHS_SD_BLK, MMCHS_SD_BLK_BLEN, 512);

		if (send_cmd_base(MMCHS_SD_CMD_CMD17 /* read single block */
		| MMCHS_SD_CMD_DP_DATA /* Command with data transfer */
		| MMCHS_SD_CMD_RSP_TYPE_48B /* type (R1) */
		| MMCHS_SD_CMD_MSBS_SINGLE /* single block */
		| MMCHS_SD_CMD_DDIR_READ /* read data from card */
		, cmd->cmdarg, cmd->resp_type)) {
			return 1;
		}

		while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT)
			& MMCHS_SD_IE_BRR_ENABLE_ENABLE) == 0) {
			count++;
		}

		if (!(read32x(MMCHS0_REG_BASE + MMCHS_SD_PSTATE) & MMCHS_SD_PSTATE_BRE_EN)) {
			return 1; /* We are not allowed to read data from the data buffer */
		}

		for (count= 0; count < 512; count+= 4) {
			value= read32x(MMCHS0_REG_BASE + MMCHS_SD_DATA);
			data->dest[count]= *((char*) &value);
			data->dest[count + 1]= *((char*) &value + 1);
			data->dest[count + 2]= *((char*) &value + 2);
			data->dest[count + 3]= *((char*) &value + 3);
		}

		while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_STAT)
			& MMCHS_SD_IE_TC_ENABLE_ENABLE) == 0) {
			count++;
		}
		write32x(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_TC_ENABLE_CLEAR);

		/* clear and disable the bbr interrupt */
		write32x(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_BRR_ENABLE_CLEAR);
		set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_BRR_ENABLE,
				MMCHS_SD_IE_BRR_ENABLE_DISABLE);
		return 0;

	} else {
		uint32_t command = cmd->cmdidx << 24;

		switch (cmd->resp_type) {
			case CARD_RSP_R1b:
				command |= MMCHS_SD_CMD_RSP_TYPE_48B_BUSY;
				break;
			case CARD_RSP_R1:
			case CARD_RSP_R3:
				command |= MMCHS_SD_CMD_RSP_TYPE_48B;
				break;
			case CARD_RSP_R2:
				command |= MMCHS_SD_CMD_RSP_TYPE_136B;
				break;
			case CARD_RSP_NONE:
				command |= MMCHS_SD_CMD_RSP_TYPE_NO_RESP;
				break;
			default:
				return 1;
		}
		ret = send_cmd_base(command, cmd->cmdarg, cmd->resp_type);

		if (ret)
		{
			return ret;
		}
	}
	

	/* copy response into cmd->resp	 */
	switch (cmd->resp_type) {
		case CARD_RSP_R1:
		case CARD_RSP_R1b:
		case CARD_RSP_R3:
			cmd->response[0] = read32x(MMCHS0_REG_BASE + MMCHS_SD_RSP10);
			break;
		case CARD_RSP_R2:
			cmd->response[3] = read32x(MMCHS0_REG_BASE + MMCHS_SD_RSP10);
			cmd->response[2] = read32x(MMCHS0_REG_BASE + MMCHS_SD_RSP32);
			cmd->response[1] = read32x(MMCHS0_REG_BASE + MMCHS_SD_RSP54);
			cmd->response[0] = read32x(MMCHS0_REG_BASE + MMCHS_SD_RSP76);
			break;
		case CARD_RSP_NONE:
			break;
		default:
			printk(BIOS_DEBUG, "Unknown case\n");
			return 1;
	}


	return 0;
}

static void set_ios(struct sd_mmc_ctrlr *ctrlr)
{

	/*
	#define SET_BUS_WIDTH(ctrlr, width)		\
	do {					\
		ctrlr->bus_width = width;	\
		ctrlr->set_ios(ctrlr);		\
	} while (0)

	#define SET_CLOCK(ctrlr, clock_hz)		\
		do {					\
			ctrlr->request_hz = clock_hz;	\
			ctrlr->set_ios(ctrlr);		\
		} while (0)

	#define SET_TIMING(ctrlr, timing_value)		\
			ctrlr->timing = timing_value;	\
			ctrlr->set_ios(ctrlr);		\
	*/

	// need to set the bus width, timing and clock

	/*
	width is 1,4,8

	#define CLOCK_KHZ		1000
#define CLOCK_MHZ		(1000 * CLOCK_KHZ)
#define CLOCK_20MHZ		(20 * CLOCK_MHZ)
#define CLOCK_25MHZ		(25 * CLOCK_MHZ)
#define CLOCK_26MHZ		(26 * CLOCK_MHZ)
#define CLOCK_50MHZ		(50 * CLOCK_MHZ)
#define CLOCK_52MHZ		(52 * CLOCK_MHZ)
#define CLOCK_200MHZ		(200 * CLOCK_MHZ)


	#define BUS_TIMING_LEGACY	0
#define BUS_TIMING_MMC_HS	1
#define BUS_TIMING_SD_HS	2
#define BUS_TIMING_UHS_SDR12	3
#define BUS_TIMING_UHS_SDR25	4
#define BUS_TIMING_UHS_SDR50	5
#define BUS_TIMING_UHS_SDR104	6
#define BUS_TIMING_UHS_DDR50	7
#define BUS_TIMING_MMC_DDR52	8
#define BUS_TIMING_MMC_HS200	9
#define BUS_TIMING_MMC_HS400	10
#define BUS_TIMING_MMC_HS400ES	11
*/
	if (ctrlr->request_hz ==CLOCK_25MHZ)
	{
		printk(BIOS_DEBUG, "setting 25mhz\n");
		set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, MMCHS_SD_SYSCTL_CEN,
			~MMCHS_SD_SYSCTL_CEN_EN);

		set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, (0x1 << 0) | (0xF << 16), (0x00 << 0) | (0xE << 16));
		set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, (0x1 << 0) | (0x3ff << 6), (0x01 << 0) | (4 << 6));

		while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL) & MMCHS_SD_SYSCTL_ICS)
			!= MMCHS_SD_SYSCTL_ICS_STABLE)
		;

		set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, MMCHS_SD_SYSCTL_CEN,
			MMCHS_SD_SYSCTL_CEN_EN);

		while ((read32x(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL) & MMCHS_SD_SYSCTL_ICS)
			!= MMCHS_SD_SYSCTL_ICS_STABLE)
		;
		
		ctrlr->bus_hz = 24000000;

	}

	printk(BIOS_DEBUG, "%s: called, bus_width: %x, clock: %d -> %d\n", __func__,
		  ctrlr->bus_width, ctrlr->request_hz, ctrlr->timing);
}

uint8_t buffer[122880];

static struct storage_media media;
static struct sd_mmc_ctrlr mmc_ctrlr;

#include <boot_device.h>
#include <symbols.h>
#include <cbfs.h>

static ssize_t unleashed_sd_readat(const struct region_device *rdev, void *dest,
					size_t offset, size_t count)
{

	// think I need to find the block that they want based on the offset
	// maybe easier just to preread it out to a buffer.. 
	printk(BIOS_DEBUG, "unleashed read %d %d\n", offset, count);

	uint8_t* dest8 = dest;

	for (int i=0; i<count; i++)
	{
		 dest8[i] = buffer[offset+i];
	}

	return count;
	
	
	//storage_block_read(&media, offset, count/512, &dest);
	//return count;
}

static const struct region_device_ops am335x_sd_ops = {
	.mmap   = mmap_helper_rdev_mmap,
	.munmap = mmap_helper_rdev_munmap,
	.readat = unleashed_sd_readat,
};


static struct mmap_helper_region_device sd_mdev =
	MMAP_HELPER_REGION_INIT(&am335x_sd_ops, 0, 122880);

const struct region_device *boot_device_ro(void)
{
	return &sd_mdev.rdev;
}

void init_sd(void)
{
	
    struct am335x_mmc *mmc = (void*)0x48060000;

	am335x_mmc_init(mmc);

	memset(&mmc_ctrlr, 0, sizeof(mmc_ctrlr));
	memset(&buffer, 0, sizeof(buffer));
	mmc_ctrlr.send_cmd = &send_cmd;
	mmc_ctrlr.set_ios = &set_ios;

	mmc_ctrlr.voltages = MMC_VDD_30_31;
	mmc_ctrlr.b_max = 1;
	//mmc_ctrlr.caps = DRVR_CAP_AUTO_CMD12 | DRVR_CAP_REMOVABLE;
	mmc_ctrlr.bus_width = 1;
	mmc_ctrlr.f_max = 24000000;
	mmc_ctrlr.f_min = 1;

	mmc_ctrlr.bus_hz = 1;

	//mmc_ctrlr.timing = BUS_TIMING_SD_HS;
	/*mmc_ctrlr.udelay_wait_after_cmd = 10000;
	mmc_ctrlr.mdelay_after_cmd0 = 100;
	mmc_ctrlr.mdelay_before_cmd0 = 100;*/

	

	media.ctrlr = &mmc_ctrlr;

	printk(BIOS_DEBUG, "pre storage\n");

	storage_setup_media(&media, &mmc_ctrlr);

	mmc_ctrlr.request_hz = CLOCK_25MHZ;
	set_ios(&mmc_ctrlr);

	printk(BIOS_DEBUG, "post storage\n");

	storage_display_setup(&media);

	storage_block_read(&media, 0, 122880/512, &buffer);

	for (int i=0x0010000; i<0x13000; i+=2)
	{

		if (i%16==0)
		{
			printk(BIOS_DEBUG, "\n%08x: ", i);
		}
		printk(BIOS_DEBUG, "%02x%02x ", buffer[i], buffer[i+1]);
	}

	mmap_helper_device_init(&sd_mdev,
			_cbfs_cache, REGION_SIZE(cbfs_cache));

	//storage_block_read(&media, 0, 1, &buffer);

	//storage_block_read(&media, 0, 1, &buffer);

	

	//	printk(BIOS_DEBUG, "post storage\n");
}