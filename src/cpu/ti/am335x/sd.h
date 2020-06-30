#ifndef AM335X_MMC_H
#define AM335X_MMC_H

#define MMCHS0_REG_BASE 0x48060000
#include <inttypes.h>

#include <commonlib/sd_mmc_ctrlr.h>

struct am335x_mmc {
	uint8_t res1[0x110];
	uint32_t sysconfig;		/* 0x110 */
	uint32_t sysstatus;		/* 0x14 */
	uint8_t res2[0x14];
	uint32_t con;		/* 0x2C */
	uint32_t pwcnt;		/* 0x30 */
	uint32_t dll;		/* 0x34 */
	uint8_t res3[0xcc];
	uint32_t blk;		/* 0x104 */
	uint32_t arg;		/* 0x108 */
	uint32_t cmd;		/* 0x10C */
	uint32_t rsp10;		/* 0x110 */
	uint32_t rsp32;		/* 0x114 */
	uint32_t rsp54;		/* 0x118 */
	uint32_t rsp76;		/* 0x11C */
	uint32_t data;		/* 0x120 */
	uint32_t pstate;		/* 0x124 */
	uint32_t hctl;		/* 0x128 */
	uint32_t sysctl;		/* 0x12C */
	uint32_t stat;		/* 0x130 */
	uint32_t ie;		/* 0x134 */
	uint8_t res4[0x4];
	uint32_t ac12;		/* 0x13C */
	uint32_t capa;		/* 0x140 */
	uint32_t capa2;		/* 0x144 */
	uint8_t res5[0xc];
	uint32_t admaes;		/* 0x154 */
	uint32_t admasal;		/* 0x158 */
};

struct am335x_mmc_host {
	struct sd_mmc_ctrlr sd_mmc_ctrlr;

	struct am335x_mmc *reg;
};



#define MMCHS_SD_SYSCONFIG_SOFTRESET                   (0x1 << 1)  /* Software reset bit writing  */


#define MMCHS_SD_SYSSTATUS_RESETDONE 0x01

#define MMCHS_SD_CON_INIT         (0x1 << 1) /* Send initialization stream (all cards) */

#define MMCHS_SD_CMD_INDX_CMD(x)          (x << 24)    /* MMC command index binary encoded values from 0 to 63 */

#define MMCHS_SD_CMD_RSP_TYPE_NO_RESP     (0x0 << 16) /* No response */
#define MMCHS_SD_CMD_RSP_TYPE_136B        (0x1 << 16) /* Response length 136 bits */
#define MMCHS_SD_CMD_RSP_TYPE_48B         (0x2 << 16) /* Response length 48 bits */
#define MMCHS_SD_CMD_RSP_TYPE_48B_BUSY    (0x3 << 16) /* Response length 48 bits with busy after response */
#define MMCHS_SD_CMD_DP_DATA              (0x1 << 21) /* Additional data is present on the data lines */
#define MMCHS_SD_CMD_DDIR_READ            (0x1 << 4)  /* Data read (card to host) */


#define MMCHS_SD_PSTATE_BRE_EN       (0x1 << 11) /* Read BLEN bytes enabled*/


#define MMCHS_SD_HCTL_DTW_1BIT       (0x0 << 1) /*1 bit transfer with */
#define MMCHS_SD_HCTL_DTW_4BIT       (0x1 << 1) /*4 bit transfer with */
#define MMCHS_SD_HCTL_SDBP           (0x1 << 8) /*SD bus power */
#define MMCHS_SD_HCTL_SDVS_VS18      (0x5 << 9) /*1.8 V */
#define MMCHS_SD_HCTL_SDVS_VS30      (0x6 << 9) /*3.0 V */
#define MMCHS_SD_HCTL_SDVS_VS33      (0x7 << 9) /*3.3 V */

#define MMCHS_SD_SYSCTL_CLKD (0x3ff << 6)  /* 10 bits clock frequency select */


#define MMCHS_SD_SYSCTL_ICE     (0x1 << 0) /* Internal clock enable register  */
#define MMCHS_SD_SYSCTL_ICS          (0x1 << 1) /* Internal clock stable register  */
#define MMCHS_SD_SYSCTL_CEN          (0x1 << 2) /* Card lock enable provide clock to the card */


#define MMCHS_SD_STAT_ERRI            (0x01 << 15) /* Error interrupt */
#define MMCHS_SD_STAT_ERROR_MASK     (0xff << 15 | 0x3 << 24 | 0x03 << 28)
#define MMCHS_SD_STAT_CC              (0x1 << 0) /* Command complete status */
#define MMCHS_SD_STAT_CC_UNRAISED     (0x0 << 0) /* Command not completed */
#define MMCHS_SD_STAT_CC_RAISED       (0x1 << 0) /* Command completed */

#define MMCHS_SD_IE_ERROR_MASK     (0xff << 15 | 0x3 << 24 | 0x03 << 28)

#define MMCHS_SD_IE_CC_ENABLE        (0x1 << 0) /* Command complete interrupt enable */
#define MMCHS_SD_IE_CC_ENABLE_ENABLE (0x1 << 0) /* Command complete Interrupts are enabled */
#define MMCHS_SD_IE_CC_ENABLE_CLEAR  (0x1 << 0) /* Clearing is done by writing a 0x1 */

#define MMCHS_SD_IE_TC_ENABLE        (0x1 << 1) /* Transfer complete interrupt enable */
#define MMCHS_SD_IE_TC_ENABLE_ENABLE (0x1 << 1) /* Transfer complete Interrupts are enabled */
#define MMCHS_SD_IE_TC_ENABLE_CLEAR  (0x1 << 1) /* Clearing TC is done by writing a 0x1 */

#define MMCHS_SD_IE_BRR_ENABLE         (0x1 << 5) /* Buffer read ready interrupt  */
#define MMCHS_SD_IE_BRR_ENABLE_DISABLE (0x0 << 5) /* Buffer read ready interrupt disable */
#define MMCHS_SD_IE_BRR_ENABLE_ENABLE  (0x1 << 5) /* Buffer read ready interrupt enable */
#define MMCHS_SD_IE_BRR_ENABLE_CLEAR   (0x1 << 5) /* Buffer read ready interrupt clear */

#define MMCHS_SD_IE_BWR_ENABLE         (0x1 << 4) /* Buffer write ready interrupt  */
#define MMCHS_SD_IE_BWR_ENABLE_DISABLE (0x0 << 4) /* Buffer write ready interrupt disable */
#define MMCHS_SD_IE_BWR_ENABLE_ENABLE  (0x1 << 4) /* Buffer write ready interrupt enable */
#define MMCHS_SD_IE_BWR_ENABLE_CLEAR   (0x1 << 4) /* Buffer write ready interrupt clear */

#define MMCHS_SD_CAPA_VS_MASK (0x7 << 24 )  /* voltage mask */
#define MMCHS_SD_CAPA_VS18 (0x01 << 26 )  /* 1.8 volt */
#define MMCHS_SD_CAPA_VS30 (0x01 << 25 )  /* 3.0 volt */
#define MMCHS_SD_CAPA_VS33 (0x01 << 24 )  /* 3.3 volt */

#define REG(x)(*((volatile uint32_t *)(x)))

void init_sd(void);

#endif	/* AM335X_UART_H */