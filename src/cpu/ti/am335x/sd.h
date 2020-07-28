#ifndef AM335X_MMC_H
#define AM335X_MMC_H

#include <inttypes.h>
#include <commonlib/sd_mmc_ctrlr.h>


/// todo: change all of these

#define SYSCONFIG_SOFTRESET (0x1 << 1)
#define SYSSTATUS_RESETDONE (0x01 << 0)
#define CON_INIT (0x1 << 1)
#define CMD_INDEX(x) (x << 24)

#define RSP_TYPE_NO_RESP     (0x0 << 16) /* No response */
#define RSP_TYPE_136B        (0x1 << 16) /* Response length 136 bits */
#define RSP_TYPE_48B         (0x2 << 16) /* Response length 48 bits */
#define RSP_TYPE_48B_BUSY    (0x3 << 16) /* Response length 48 bits with busy after response */
#define DP_DATA              (0x1 << 21) /* Additional data is present on the data lines */
#define DDIR_READ            (0x1 << 4)  /* Data read (card to host) */
#define HCTL_DTW_1BIT       (0x0 << 1) /*1 bit transfer with */
#define HCTL_SDBP           (0x1 << 8) /*SD bus power */
#define HCTL_SDVS_VS30      (0x6 << 9) /*3.0 V */

#define SYSCTL_ICE     	    (0x1 << 0) /* Internal clock enable register  */
#define SYSCTL_ICS          (0x1 << 1) /* Internal clock stable register  */
#define SYSCTL_CEN          (0x1 << 2) /* Card lock enable provide clock to the card */

#define STAT_ERRI            (0x01 << 15) /* Error interrupt */
#define STAT_ERROR_MASK     (0xff << 15 | 0x3 << 24 | 0x03 << 28)
#define STAT_CC              (0x1 << 0) /* Command complete status */

#define IE_CC        (0x1 << 0) /* Command complete interrupt enable */
#define IE_TC        (0x1 << 1) /* Transfer complete interrupt enable */


#define IE_BRR         (0x1 << 5) /* Buffer read ready interrupt  */
#define MMCHS_SD_IE_BRR_ENABLE_DISABLE (0x0 << 5) /* Buffer read ready interrupt disable */
#define MMCHS_SD_IE_BRR_ENABLE_ENABLE  (0x1 << 5) /* Buffer read ready interrupt enable */
#define MMCHS_SD_IE_BRR_ENABLE_CLEAR   (0x1 << 5) /* Buffer read ready interrupt clear */

#define CAPA_VS18 (0x01 << 26 )  /* 1.8 volt */
#define CAPA_VS30 (0x01 << 25 )  /* 3.0 volt */

#define MMCHS0_REG_BASE 0x48060000

struct am335x_mmc {
	uint8_t res1[0x110];
	uint32_t sysconfig;
	uint32_t sysstatus;
	uint8_t res2[0x14];
	uint32_t con;
	uint32_t pwcnt;
	uint32_t dll;
	uint8_t res3[0xcc];
	uint32_t blk;
	uint32_t arg;
	uint32_t cmd;
	uint32_t rsp10;
	uint32_t rsp32;
	uint32_t rsp54;
	uint32_t rsp76;
	uint32_t data;
	uint32_t pstate;
	uint32_t hctl;
	uint32_t sysctl;
	uint32_t stat;	
	uint32_t ie;
	uint8_t res4[0x4];
	uint32_t ac12;
	uint32_t capa;
	uint32_t capa2;
	uint8_t res5[0xc];
	uint32_t admaes;
	uint32_t admasal;
};

struct am335x_mmc_host {
	struct sd_mmc_ctrlr sd_mmc_ctrlr;
	struct am335x_mmc *reg;
	uint32_t sd_clock_hz;
};

int am335x_sd_init_storage(struct am335x_mmc_host * mmc_host);

#endif	/* AM335X_UART_H */