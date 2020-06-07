#include <stdio.h>
#include <inttypes.h>
#include <console/console.h>

#include "sdmmc.h"



struct sd_card
{
	uint32_t cid[4]; /* Card Identification */
	uint32_t rca; /* Relative card address */
	uint32_t dsr; /* Driver stage register */
	uint32_t csd[4]; /* Card specific data */
	uint32_t scr[2]; /* SD configuration */
	uint32_t ocr; /* Operation conditions */
	uint32_t ssr[5]; /* SD Status */
	uint32_t csr; /* Card status */
};

/**
 * Write a uint32_t value to a memory address
 */
static inline void write32(uint32_t address, uint32_t value)
{
	REG(address)= value;
}

/**
 * Read an uint32_t from a memory address
 */
static inline uint32_t read32(uint32_t address)
{
	return REG(address);
}

/**
 * Set a 32 bits value depending on a mask
 */
static inline void set32(uint32_t address, uint32_t mask, uint32_t value)
{
	uint32_t val;
	val= read32(address);
	val&= ~(mask); /* clear the bits */
	val|= (value & mask); /* apply the value using the mask */
	write32(address, val);
}

static int mmchs_init(void)
{

	int counter;
	counter= 0;

	/*
	 * Soft reset of the controller
	 */
	/* Write 1 to sysconfig[0] to trigger a reset*/
	set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCONFIG, MMCHS_SD_SYSCONFIG_SOFTRESET,
			MMCHS_SD_SYSCONFIG_SOFTRESET);

	/* read sysstatus to know it's done */
	while (!(read32(MMCHS0_REG_BASE + MMCHS_SD_SYSSTATUS)
			& MMCHS_SD_SYSSTATUS_RESETDONE)) {
		counter++;
	}

	/*
	 * Set SD default capabilities
	 */
	set32(MMCHS0_REG_BASE + MMCHS_SD_CAPA, MMCHS_SD_CAPA_VS_MASK,
			MMCHS_SD_CAPA_VS18 | MMCHS_SD_CAPA_VS30);

	/*
	 * wake-up configuration
	 */
	set32(
			MMCHS0_REG_BASE + MMCHS_SD_SYSCONFIG,
			MMCHS_SD_SYSCONFIG_AUTOIDLE | MMCHS_SD_SYSCONFIG_ENAWAKEUP
					| MMCHS_SD_SYSCONFIG_STANDBYMODE
					| MMCHS_SD_SYSCONFIG_CLOCKACTIVITY
					| MMCHS_SD_SYSCONFIG_SIDLEMODE,
			MMCHS_SD_SYSCONFIG_AUTOIDLE_EN /* Automatic clock gating strategy */
			| MMCHS_SD_SYSCONFIG_ENAWAKEUP_EN /*  Enable wake-up capability */
			| MMCHS_SD_SYSCONFIG_SIDLEMODE_IDLE /*  Smart-idle */
			| MMCHS_SD_SYSCONFIG_CLOCKACTIVITY_OFF /* Booth the interface and functional can be switched off */
			| MMCHS_SD_SYSCONFIG_STANDBYMODE_WAKEUP_INTERNAL /* Go info wake-up mode when possible */
			);

	/* Wake-up on sd interrupt SDIO */
	set32(MMCHS0_REG_BASE + MMCHS_SD_HCTL, MMCHS_SD_HCTL_IWE,
			MMCHS_SD_HCTL_IWE_EN);

	/*
	 * MMC host and bus configuration
	 */

	/* Configure data and command transfer (1 bit mode)*/
	set32(MMCHS0_REG_BASE + MMCHS_SD_CON, MMCHS_SD_CON_DW8,
			MMCHS_SD_CON_DW8_1BIT);
	set32(MMCHS0_REG_BASE + MMCHS_SD_HCTL, MMCHS_SD_HCTL_DTW,
			MMCHS_SD_HCTL_DTW_1BIT);

	/* Configure card voltage  */
	set32(MMCHS0_REG_BASE + MMCHS_SD_HCTL, MMCHS_SD_HCTL_SDVS,
			MMCHS_SD_HCTL_SDVS_VS30 /* Configure 3.0 volt */
			);

	/* Power on the host controller and wait for the  MMCHS_SD_HCTL_SDBP_POWER_ON to be set */
	set32(MMCHS0_REG_BASE + MMCHS_SD_HCTL, MMCHS_SD_HCTL_SDBP,
			MMCHS_SD_HCTL_SDBP_ON);

	while ((read32(MMCHS0_REG_BASE + MMCHS_SD_HCTL) & MMCHS_SD_HCTL_SDBP)
			!= MMCHS_SD_HCTL_SDBP_ON) {
		counter++;
	}

	/* Enable internal clock and clock to the card*/
	set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, MMCHS_SD_SYSCTL_ICE,
			MMCHS_SD_SYSCTL_ICE_EN);

	//@TODO Fix external clock enable , this one is very slow
	set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, MMCHS_SD_SYSCTL_CLKD,
			(0x3ff << 6));
	set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, MMCHS_SD_SYSCTL_CEN,
			MMCHS_SD_SYSCTL_CEN_EN);
	while ((read32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL) & MMCHS_SD_SYSCTL_ICS)
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
	write32(MMCHS0_REG_BASE + MMCHS_SD_CMD, 0x00); /* command 0 , type other commands , not response etc) */

	while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT) & MMCHS_SD_STAT_CC)
			!= MMCHS_SD_STAT_CC_RAISED) {
		if (read32(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0x8000) {
			printk(BIOS_DEBUG,"%s, error stat  %x\n", __FUNCTION__,
					read32(MMCHS0_REG_BASE + MMCHS_SD_STAT));
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

static int send_cmd(uint32_t command, uint32_t arg)
{
	int count= 0;

	/* Read current interrupt status and fail it an interrupt is already asserted */
	if ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0xffffu)) {
		printk(BIOS_DEBUG,"%s, interrupt already raised stat  %08x\n", __FUNCTION__,
				read32(MMCHS0_REG_BASE + MMCHS_SD_STAT));
		return 1;
	}

	/* Set arguments */
	write32(MMCHS0_REG_BASE + MMCHS_SD_ARG, arg);
	/* Set command */
	set32(MMCHS0_REG_BASE + MMCHS_SD_CMD, MMCHS_SD_CMD_MASK, command);

	/* Wait for completion */
	while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0xffffu) == 0x0) {
		count++;
	}

	if (read32(MMCHS0_REG_BASE + MMCHS_SD_STAT) & 0x8000) {
		printk(BIOS_DEBUG,"%s, error stat  %08x\n", __FUNCTION__,
				read32(MMCHS0_REG_BASE + MMCHS_SD_STAT));
		set32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_STAT_ERROR_MASK,
				0xffffffffu);	// clear errors
		// We currently only support 2.0, not responding to
		return 1;
	}

	if ((command & MMCHS_SD_CMD_RSP_TYPE) == MMCHS_SD_CMD_RSP_TYPE_48B_BUSY) {
		/*
		 * Command with busy repsonse *CAN* also set the TC bit if they exit busy
		 */
		while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT)
				& MMCHS_SD_IE_TC_ENABLE_ENABLE) == 0) {
			count++;
		}
		write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_TC_ENABLE_CLEAR);
	}

	/* clear the cc status */
	write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_CC_ENABLE_CLEAR);
	return 0;
}

static int card_goto_idle_state(void)
{
	if (send_cmd(0x00, 0x00)) {
		return 1;
	}
	return 0;
}

static int card_identification(void)
{
	uint32_t value;

	if (send_cmd(MMCHS_SD_CMD_CMD8 | MMCHS_SD_CMD_RSP_TYPE_48B,
			MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN)) {
		// We currently only support 2.0,
		return 1;
	}

	/* Check the pattern and CRC */
	value= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP10);
	if (!(value == (MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN))) {
		printk(BIOS_DEBUG,"card_identification, check pattern check failed\n");
		return 1;
	}
	return 0;
}

static int card_query_voltage_and_type(struct sd_card * card)
{
	uint32_t value;

	if (send_cmd(MMCHS_SD_CMD_CMD55 | MMCHS_SD_CMD_RSP_TYPE_48B, 0)) { /* RCA=0000 */
		return 1;
	}
	/* Send ADMD41 */
	/* 0x1 << 30 == send HCS (Host capacity support) and get OCR register */
	if (send_cmd(SD_SEND_OP_COND | MMCHS_SD_CMD_RSP_TYPE_48B,
			(0x3F << 15) | (0x1 << 30))) {
		return 1;
	}

	while (1) {		//@TODO wait for max 1
		if (send_cmd(MMCHS_SD_CMD_CMD55 | MMCHS_SD_CMD_RSP_TYPE_48B, 0)) { /* RCA=0000 */
			return 1;
		}
		/* Send ADMD41 */
		/* 0x1 << 30 == send HCS (Host capacity support) and get OCR register */
		if (send_cmd(SD_SEND_OP_COND | MMCHS_SD_CMD_RSP_TYPE_48B,
				(0x1FF << 15) | (0x1 << 30))) {
			return 1;
		}

		value= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP10);
		if ((value & (0x1u << 31))) {
			break;
		}
	}
	card->ocr= value;
	return 0;
}

static int card_identify(struct sd_card * card)
{

	/* Send cmd 2 (all_send_cid) and expect 136 bits response*/
	if (send_cmd(MMCHS_SD_CMD_CMD2 | MMCHS_SD_CMD_RSP_TYPE_136B, 0)) {
		return 1;
	}

	card->cid[0]= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP10);
	card->cid[1]= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP32);
	card->cid[2]= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP54);
	card->cid[3]= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP76);

	/* R6 response */
	if (send_cmd(MMCHS_SD_CMD_CMD3 | MMCHS_SD_CMD_RSP_TYPE_48B, 0)) {
		return 1;
	}
	card->rca= (read32(MMCHS0_REG_BASE + MMCHS_SD_RSP10) & 0xffff0000u) >> 16;

	/* MMHCS only supports a single card so sending MMCHS_SD_CMD_CMD2 is useless
	 * Still we should make it possible in the API to support multiple cards
	 */

	return 0;
}

static int card_csd(struct sd_card *card)
{
	/* send_csd -> r2 response */
	if (send_cmd(MMCHS_SD_CMD_CMD9 | MMCHS_SD_CMD_RSP_TYPE_136B,
			(card->rca << 16))) {
		return 1;
	}
	card->csd[0]= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP10);
	card->csd[1]= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP32);
	card->csd[2]= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP54);
	card->csd[3]= read32(MMCHS0_REG_BASE + MMCHS_SD_RSP76);

	/* sanity check */
	if (((card->csd[3] >> 30) & 0x3) != 0x1) {
		printk(BIOS_DEBUG,"Version 2.0 of CSD register expected\n");
		return 1;
	}
	long c_size= (card->csd[1] >> 16) | ((card->csd[2] & 0x3F) << 16);
	printk(BIOS_DEBUG,"size = %lu\n", (c_size + 1) * 512 * 1024);
	return 0;
}

static int select_card(struct sd_card *card)
{
	/* select card */
	if (send_cmd(MMCHS_SD_CMD_CMD7 | MMCHS_SD_CMD_RSP_TYPE_48B_BUSY,
			(card->rca << 16))) {
		return 1;
	}
	return 0;
}

static int read_single_block(struct sd_card *card, uint32_t blknr, unsigned char * buf)
{
	uint32_t count;
	uint32_t value;

	count= 0;

	set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_BRR_ENABLE,
			MMCHS_SD_IE_BRR_ENABLE_ENABLE);

	set32(MMCHS0_REG_BASE + MMCHS_SD_BLK, MMCHS_SD_BLK_BLEN, 512);

	//  send cmd 17 0 21

	if (send_cmd(MMCHS_SD_CMD_CMD17 /* read single block */
	| MMCHS_SD_CMD_DP_DATA /* Command with data transfer */
	| MMCHS_SD_CMD_RSP_TYPE_48B /* type (R1) */
	| MMCHS_SD_CMD_MSBS_SINGLE /* single block */
	| MMCHS_SD_CMD_DDIR_READ /* read data from card */
	, blknr)) {
		return 1;
	}

	while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT)
			& MMCHS_SD_IE_BRR_ENABLE_ENABLE) == 0) {
		count++;
	}

	if (!(read32(MMCHS0_REG_BASE + MMCHS_SD_PSTATE) & MMCHS_SD_PSTATE_BRE_EN)) {
		return 1; /* We are not allowed to read data from the data buffer */
	}

	for (count= 0; count < 512; count+= 4) {
		value= read32(MMCHS0_REG_BASE + MMCHS_SD_DATA);
		buf[count]= *((char*) &value);
		buf[count + 1]= *((char*) &value + 1);
		buf[count + 2]= *((char*) &value + 2);
		buf[count + 3]= *((char*) &value + 3);
	}

	/* Wait for TC */

	while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT)
			& MMCHS_SD_IE_TC_ENABLE_ENABLE) == 0) {
		count++;
	}
	write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_TC_ENABLE_CLEAR);

	/* clear and disable the bbr interrupt */
	write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_BRR_ENABLE_CLEAR);
	set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_BRR_ENABLE,
			MMCHS_SD_IE_BRR_ENABLE_DISABLE);
	return 0;
}

static int write_single_block(struct sd_card *card,
		uint32_t blknr,
		unsigned char * buf)
{
	uint32_t count;
	uint32_t value;

	count= 0;

	set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_BWR_ENABLE,
			MMCHS_SD_IE_BWR_ENABLE_ENABLE);
	//set32(MMCHS0_REG_BASE + MMCHS_SD_IE, 0xfff , 0xfff);
	set32(MMCHS0_REG_BASE + MMCHS_SD_BLK, MMCHS_SD_BLK_BLEN, 512);

	/* Set timeout */
	set32(MMCHS0_REG_BASE + MMCHS_SD_SYSCTL, MMCHS_SD_SYSCTL_DTO,
			MMCHS_SD_SYSCTL_DTO_2POW27);

	if (send_cmd(MMCHS_SD_CMD_CMD24 /* write single block */
	| MMCHS_SD_CMD_DP_DATA /* Command with data transfer */
	| MMCHS_SD_CMD_RSP_TYPE_48B /* type (R1b) */
	| MMCHS_SD_CMD_MSBS_SINGLE /* single block */
	| MMCHS_SD_CMD_DDIR_WRITE /* write to the card */
	, blknr)) {
		return 1;
	}

	/* Wait for the MMCHS_SD_IE_BWR_ENABLE interrupt */
	while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT) & MMCHS_SD_IE_BWR_ENABLE)
			== 0) {
		count++;
	}

	if (!(read32(MMCHS0_REG_BASE + MMCHS_SD_PSTATE) & MMCHS_SD_PSTATE_BWE_EN)) {
		return 1; /* not ready to write data */
	}
	for (count= 0; count < 512; count+= 4) {
		*((char*) &value)= buf[count];
		*((char*) &value + 1)= buf[count + 1];
		*((char*) &value + 2)= buf[count + 2];
		*((char*) &value + 3)= buf[count + 3];
		write32(MMCHS0_REG_BASE + MMCHS_SD_DATA, value);
	}

	/* Wait for TC */
	while ((read32(MMCHS0_REG_BASE + MMCHS_SD_STAT)
			& MMCHS_SD_IE_TC_ENABLE_ENABLE) == 0) {
		count++;
	}
	write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_TC_ENABLE_CLEAR);
	write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_CC_ENABLE_CLEAR);/* finished.  */
	/* clear the bwr interrupt FIXME is this right when writing?*/
	write32(MMCHS0_REG_BASE + MMCHS_SD_STAT, MMCHS_SD_IE_BWR_ENABLE_CLEAR);
	set32(MMCHS0_REG_BASE + MMCHS_SD_IE, MMCHS_SD_IE_BWR_ENABLE,
			MMCHS_SD_IE_BWR_ENABLE_DISABLE);
	return 0;
}

int sd_init(void)
{
	struct sd_card card;
	int i;

	unsigned char buf[1024];
	for (i= 0; i < 1024; i++) {
		buf[i]= 0x0;
	}

	if (mmchs_init()) {
		printk(BIOS_DEBUG,"Failed to initialize the host controller\n");
		return 1;
	}

	return 0;

	if (card_goto_idle_state()) {
		printk(BIOS_DEBUG,"failed to go into idle state\n");
		return 1;
	}
	if (card_identification()) {
		printk(BIOS_DEBUG,"failed to go card_identification\n");
		return 1;
	}
	if (card_query_voltage_and_type(&card)) {
		printk(BIOS_DEBUG,"failed to go card_query_voltage_and_type\n");
		return 1;
	}
	if (card_identify(&card)) {
		printk(BIOS_DEBUG,"failed to identify card\n");
		return 1;
	}
	/* We have now initialized the hardware identified the card */
	if (card_csd(&card)) {
		printk(BIOS_DEBUG,"failed to read csd (card specific data)\n");
		return 1;
	}
	if (select_card(&card)) {
		printk(BIOS_DEBUG,"Failed to select card\n");
		return 1;
	}

	if (read_single_block(&card, 0, buf)) {
		printk(BIOS_DEBUG,"Failed to read a single block\n");
		return 1;
	}

	/* check signature */
	if (!(buf[0x01fe] == 0x55 && buf[0x01ff] == 0xaa)) {
		printk(BIOS_DEBUG,"Failed to find MBR signature\n");
		return 1;
	}

	for (i= 0; i < 512; i++) {
		buf[i]= i % 256;
	}

	if (read_single_block(&card, 0, buf)) {
		printk(BIOS_DEBUG,"Failed to read a single block\n");
		return 1;
	}

	/* check signature */
	if (!(buf[0x01fe] == 0x55 && buf[0x01ff] == 0xaa)) {
		printk(BIOS_DEBUG,"Failed to find MBR signature\n");
		return 1;
	}

	/* DESCUCTIVE... */

	for (i= 0; i < 512; i++) {
		buf[i]= i % 256;
	}

	if (write_single_block(&card, 0xfffff, buf)) {
		printk(BIOS_DEBUG,"Failed to write a single block\n");
		return 1;
	}

	for (i= 0; i < 512; i++) {
		buf[i]= 0;
	}

	if (read_single_block(&card, 0xfffff, buf)) {
		printk(BIOS_DEBUG,"Failed to write a single block (check)\n");
		return 1;
	}

	for (i= 0; i < 512; i++) {
		if ((!buf[i]) == i % 256) {
			printk(BIOS_DEBUG,"Failed to write a single block and read it again \n");
			return 1;
		}
	}

	return 0;
}