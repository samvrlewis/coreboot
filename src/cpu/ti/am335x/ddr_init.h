#ifndef __CPU_TI_AM335X_DDR_INIT_H__
#define __CPU_TI_AM335X_DDR_INIT_H__
#include <types.h>

/* AM335X EMIF Register values */
#define VTP_CTRL_READY		(0x1 << 5)
#define VTP_CTRL_ENABLE		(0x1 << 6)
#define VTP_CTRL_START_EN	(0x1)

#define DDR_CKE_CTRL_NORMAL	0x1

#define PHY_EN_DYN_PWRDN	(0x1 << 20)


struct ctrl_ioregs {
	unsigned int cm0ioctl;
	unsigned int cm1ioctl;
	unsigned int cm2ioctl;
	unsigned int dt0ioctl;
	unsigned int dt1ioctl;
	unsigned int dt2ioctrl;
	unsigned int dt3ioctrl;
	unsigned int emif_sdram_config_ext;
};

/**
 * Encapsulates DDR DATA registers.
 */
struct ddr_data {
	unsigned long datardsratio0;
	unsigned long datawdsratio0;
	unsigned long datawiratio0;
	unsigned long datagiratio0;
	unsigned long datafwsratio0;
	unsigned long datawrsratio0;
};

/**
 * Encapsulates DDR CMD control registers.
 */
struct cmd_control {
	unsigned long cmd0csratio;
	unsigned long cmd0csforce;
	unsigned long cmd0csdelay;
	unsigned long cmd0iclkout;
	unsigned long cmd1csratio;
	unsigned long cmd1csforce;
	unsigned long cmd1csdelay;
	unsigned long cmd1iclkout;
	unsigned long cmd2csratio;
	unsigned long cmd2csforce;
	unsigned long cmd2csdelay;
	unsigned long cmd2iclkout;
};


/*
 * Structure containing shadow of important registers in EMIF
 * The calculation function fills in this structure to be later used for
 * initialization and DVFS
 */
struct emif_regs {
	u32 freq;
	u32 sdram_config_init;
	u32 sdram_config;
	u32 sdram_config2;
	u32 ref_ctrl;
	u32 ref_ctrl_final;
	u32 sdram_tim1;
	u32 sdram_tim2;
	u32 sdram_tim3;
	u32 ocp_config;
	u32 read_idle_ctrl;
	u32 zq_config;
	u32 temp_alert_config;
	u32 emif_ddr_phy_ctlr_1_init;
	u32 emif_ddr_phy_ctlr_1;
	u32 emif_ddr_ext_phy_ctrl_1;
	u32 emif_ddr_ext_phy_ctrl_2;
	u32 emif_ddr_ext_phy_ctrl_3;
	u32 emif_ddr_ext_phy_ctrl_4;
	u32 emif_ddr_ext_phy_ctrl_5;
	u32 emif_rd_wr_lvl_rmp_win;
	u32 emif_rd_wr_lvl_rmp_ctl;
	u32 emif_rd_wr_lvl_ctl;
	u32 emif_rd_wr_exec_thresh;
	u32 emif_prio_class_serv_map;
	u32 emif_connect_id_serv_1_map;
	u32 emif_connect_id_serv_2_map;
	u32 emif_cos_config;
	u32 emif_ecc_ctrl_reg;
	u32 emif_ecc_address_range_1;
	u32 emif_ecc_address_range_2;
};

/* VTP Registers */
struct vtp_reg {
	unsigned int vtp0ctrlreg;
};


/* Reg mapping structure */
struct emif_reg_struct {
	u32 emif_mod_id_rev;
	u32 emif_status;
	u32 emif_sdram_config;
	u32 emif_lpddr2_nvm_config;
	u32 emif_sdram_ref_ctrl;
	u32 emif_sdram_ref_ctrl_shdw;
	u32 emif_sdram_tim_1;
	u32 emif_sdram_tim_1_shdw;
	u32 emif_sdram_tim_2;
	u32 emif_sdram_tim_2_shdw;
	u32 emif_sdram_tim_3;
	u32 emif_sdram_tim_3_shdw;
	u32 emif_lpddr2_nvm_tim;
	u32 emif_lpddr2_nvm_tim_shdw;
	u32 emif_pwr_mgmt_ctrl;
	u32 emif_pwr_mgmt_ctrl_shdw;
	u32 emif_lpddr2_mode_reg_data;
	u32 padding1[1];
	u32 emif_lpddr2_mode_reg_data_es2;
	u32 padding11[1];
	u32 emif_lpddr2_mode_reg_cfg;
	u32 emif_l3_config;
	u32 emif_l3_cfg_val_1;
	u32 emif_l3_cfg_val_2;
	u32 emif_iodft_tlgc;
	u32 padding2[7];
	u32 emif_perf_cnt_1;
	u32 emif_perf_cnt_2;
	u32 emif_perf_cnt_cfg;
	u32 emif_perf_cnt_sel;
	u32 emif_perf_cnt_tim;
	u32 padding3;
	u32 emif_read_idlectrl;
	u32 emif_read_idlectrl_shdw;
	u32 padding4;
	u32 emif_irqstatus_raw_sys;
	u32 emif_irqstatus_raw_ll;
	u32 emif_irqstatus_sys;
	u32 emif_irqstatus_ll;
	u32 emif_irqenable_set_sys;
	u32 emif_irqenable_set_ll;
	u32 emif_irqenable_clr_sys;
	u32 emif_irqenable_clr_ll;
	u32 padding5;
	u32 emif_zq_config;
	u32 emif_temp_alert_config;
	u32 emif_l3_err_log;
	u32 emif_rd_wr_lvl_rmp_win;
	u32 emif_rd_wr_lvl_rmp_ctl;
	u32 emif_rd_wr_lvl_ctl;
	u32 padding6[1];
	u32 emif_ddr_phy_ctrl_1;
	u32 emif_ddr_phy_ctrl_1_shdw;
	u32 emif_ddr_phy_ctrl_2;
	u32 padding7[4];
	u32 emif_prio_class_serv_map;
	u32 emif_connect_id_serv_1_map;
	u32 emif_connect_id_serv_2_map;
	u32 padding8;
	u32 emif_ecc_ctrl_reg;
	u32 emif_ecc_address_range_1;
	u32 emif_ecc_address_range_2;
	u32 padding8_1;
	u32 emif_rd_wr_exec_thresh;
	u32 emif_cos_config;
#if defined(CONFIG_DRA7XX) || defined(CONFIG_ARCH_KEYSTONE)
	u32 padding9[2];
	u32 emif_1b_ecc_err_cnt;
	u32 emif_1b_ecc_err_thrush;
	u32 emif_1b_ecc_err_dist_1;
	u32 emif_1b_ecc_err_addr_log;
	u32 emif_2b_ecc_err_addr_log;
	u32 emif_ddr_phy_status[28];
	u32 padding10[19];
#else
	u32 padding9[6];
	u32 emif_ddr_phy_status[28];
	u32 padding10[20];
#endif
	u32 emif_ddr_ext_phy_ctrl_1;
	u32 emif_ddr_ext_phy_ctrl_1_shdw;
	u32 emif_ddr_ext_phy_ctrl_2;
	u32 emif_ddr_ext_phy_ctrl_2_shdw;
	u32 emif_ddr_ext_phy_ctrl_3;
	u32 emif_ddr_ext_phy_ctrl_3_shdw;
	u32 emif_ddr_ext_phy_ctrl_4;
	u32 emif_ddr_ext_phy_ctrl_4_shdw;
	u32 emif_ddr_ext_phy_ctrl_5;
	u32 emif_ddr_ext_phy_ctrl_5_shdw;
	u32 emif_ddr_ext_phy_ctrl_6;
	u32 emif_ddr_ext_phy_ctrl_6_shdw;
	u32 emif_ddr_ext_phy_ctrl_7;
	u32 emif_ddr_ext_phy_ctrl_7_shdw;
	u32 emif_ddr_ext_phy_ctrl_8;
	u32 emif_ddr_ext_phy_ctrl_8_shdw;
	u32 emif_ddr_ext_phy_ctrl_9;
	u32 emif_ddr_ext_phy_ctrl_9_shdw;
	u32 emif_ddr_ext_phy_ctrl_10;
	u32 emif_ddr_ext_phy_ctrl_10_shdw;
	u32 emif_ddr_ext_phy_ctrl_11;
	u32 emif_ddr_ext_phy_ctrl_11_shdw;
	u32 emif_ddr_ext_phy_ctrl_12;
	u32 emif_ddr_ext_phy_ctrl_12_shdw;
	u32 emif_ddr_ext_phy_ctrl_13;
	u32 emif_ddr_ext_phy_ctrl_13_shdw;
	u32 emif_ddr_ext_phy_ctrl_14;
	u32 emif_ddr_ext_phy_ctrl_14_shdw;
	u32 emif_ddr_ext_phy_ctrl_15;
	u32 emif_ddr_ext_phy_ctrl_15_shdw;
	u32 emif_ddr_ext_phy_ctrl_16;
	u32 emif_ddr_ext_phy_ctrl_16_shdw;
	u32 emif_ddr_ext_phy_ctrl_17;
	u32 emif_ddr_ext_phy_ctrl_17_shdw;
	u32 emif_ddr_ext_phy_ctrl_18;
	u32 emif_ddr_ext_phy_ctrl_18_shdw;
	u32 emif_ddr_ext_phy_ctrl_19;
	u32 emif_ddr_ext_phy_ctrl_19_shdw;
	u32 emif_ddr_ext_phy_ctrl_20;
	u32 emif_ddr_ext_phy_ctrl_20_shdw;
	u32 emif_ddr_ext_phy_ctrl_21;
	u32 emif_ddr_ext_phy_ctrl_21_shdw;
	u32 emif_ddr_ext_phy_ctrl_22;
	u32 emif_ddr_ext_phy_ctrl_22_shdw;
	u32 emif_ddr_ext_phy_ctrl_23;
	u32 emif_ddr_ext_phy_ctrl_23_shdw;
	u32 emif_ddr_ext_phy_ctrl_24;
	u32 emif_ddr_ext_phy_ctrl_24_shdw;
	u32 emif_ddr_ext_phy_ctrl_25;
	u32 emif_ddr_ext_phy_ctrl_25_shdw;
	u32 emif_ddr_ext_phy_ctrl_26;
	u32 emif_ddr_ext_phy_ctrl_26_shdw;
	u32 emif_ddr_ext_phy_ctrl_27;
	u32 emif_ddr_ext_phy_ctrl_27_shdw;
	u32 emif_ddr_ext_phy_ctrl_28;
	u32 emif_ddr_ext_phy_ctrl_28_shdw;
	u32 emif_ddr_ext_phy_ctrl_29;
	u32 emif_ddr_ext_phy_ctrl_29_shdw;
	u32 emif_ddr_ext_phy_ctrl_30;
	u32 emif_ddr_ext_phy_ctrl_30_shdw;
	u32 emif_ddr_ext_phy_ctrl_31;
	u32 emif_ddr_ext_phy_ctrl_31_shdw;
	u32 emif_ddr_ext_phy_ctrl_32;
	u32 emif_ddr_ext_phy_ctrl_32_shdw;
	u32 emif_ddr_ext_phy_ctrl_33;
	u32 emif_ddr_ext_phy_ctrl_33_shdw;
	u32 emif_ddr_ext_phy_ctrl_34;
	u32 emif_ddr_ext_phy_ctrl_34_shdw;
	u32 emif_ddr_ext_phy_ctrl_35;
	u32 emif_ddr_ext_phy_ctrl_35_shdw;
	union {
		u32 emif_ddr_ext_phy_ctrl_36;
		u32 emif_ddr_fifo_misaligned_clear_1;
	};
	union {
		u32 emif_ddr_ext_phy_ctrl_36_shdw;
		u32 emif_ddr_fifo_misaligned_clear_2;
	};
};

struct ddr_cmd_regs {
	unsigned int resv0[7];
	unsigned int cm0csratio;	/* offset 0x01C */
	unsigned int resv1[3];
	unsigned int cm0iclkout;	/* offset 0x02C */
	unsigned int resv2[8];
	unsigned int cm1csratio;	/* offset 0x050 */
	unsigned int resv3[3];
	unsigned int cm1iclkout;	/* offset 0x060 */
	unsigned int resv4[8];
	unsigned int cm2csratio;	/* offset 0x084 */
	unsigned int resv5[3];
	unsigned int cm2iclkout;	/* offset 0x094 */
	unsigned int resv6[3];
};

struct ddr_data_regs {
	unsigned int dt0rdsratio0;	/* offset 0x0C8 */
	unsigned int resv1[4];
	unsigned int dt0wdsratio0;	/* offset 0x0DC */
	unsigned int resv2[4];
	unsigned int dt0wiratio0;	/* offset 0x0F0 */
	unsigned int resv3;
	unsigned int dt0wimode0;	/* offset 0x0F8 */
	unsigned int dt0giratio0;	/* offset 0x0FC */
	unsigned int resv4;
	unsigned int dt0gimode0;	/* offset 0x104 */
	unsigned int dt0fwsratio0;	/* offset 0x108 */
	unsigned int resv5[4];
	unsigned int dt0dqoffset;	/* offset 0x11C */
	unsigned int dt0wrsratio0;	/* offset 0x120 */
	unsigned int resv6[4];
	unsigned int dt0rdelays0;	/* offset 0x134 */
	unsigned int dt0dldiff0;	/* offset 0x138 */
	unsigned int resv7[12];
};

/* Control Status Register */
struct ctrl_stat {
	unsigned int resv1[16];
	unsigned int statusreg;		/* ofset 0x40 */
	unsigned int resv2[51];
	unsigned int secure_emif_sdram_config;	/* offset 0x0110 */
	unsigned int resv3[319];
	unsigned int dev_attr;
};

/**
 * This structure represents the DDR io control on AM33XX devices.
 */
struct ddr_cmdtctrl {
	unsigned int cm0ioctl;
	unsigned int cm1ioctl;
	unsigned int cm2ioctl;
	unsigned int resv2[12];
	unsigned int dt0ioctl;
	unsigned int dt1ioctl;
	unsigned int dt2ioctrl;
	unsigned int dt3ioctrl;
	unsigned int resv3[4];
	unsigned int emif_sdram_config_ext;
};

struct ddr_ctrl {
	unsigned int ddrioctrl;
	unsigned int resv1[325];
	unsigned int ddrckectrl;
};


/* VTP Base address */
#define VTP0_CTRL_ADDR			0x44E10E0C
#define VTP1_CTRL_ADDR			0x48140E10

/* EMIF Base address */
#define EMIF4_0_CFG_BASE		0x4C000000
#define EMIF4_1_CFG_BASE		0x4D000000

/* DDR Base address */
#define DDR_PHY_CMD_ADDR		0x44E12000
#define DDR_PHY_DATA_ADDR		0x44E120C8
#define DDR_PHY_CMD_ADDR2		0x47C0C800
#define DDR_PHY_DATA_ADDR2		0x47C0C8C8
#define DDR_DATA_REGS_NR		2

/* DDR Base address */
#define DDR_CTRL_ADDR			0x44E10E04
#define DDR_CONTROL_BASE_ADDR	0x44E11404

/* Control Module Base Address */
#define CTRL_BASE			0x44E10000

#define EMIF_REG_MAJOR_REVISION_SHIFT		8
#define EMIF_REG_MAJOR_REVISION_MASK		(0x7 << 8)

#define EMIF_REG_SDRAM_TYPE_SHIFT			29
#define EMIF_REG_SDRAM_TYPE_MASK			(0x7 << 29)

#define EMIF_EXT_PHY_CTRL_TIMING_REG	0x5

#define EMIF_REG_INITREF_DIS_MASK			(1 << 31)
#define EMIF_4D5				0x5

/* SDRAM TYPE */
#define EMIF_SDRAM_TYPE_DDR2	0x2
#define EMIF_SDRAM_TYPE_DDR3	0x3
#define EMIF_SDRAM_TYPE_LPDDR2	0x4

void config_ddr(unsigned int pll, const struct ctrl_ioregs *ioregs,
		const struct ddr_data *data, const struct cmd_control *ctrl,
		const struct emif_regs *regs, int nr);

#define PRCM_MOD_EN		0x2

/**
 * Encapsulating peripheral functional clocks
 * pll registers
 */
struct cm_perpll {
	unsigned int l4lsclkstctrl;	/* offset 0x00 */
	unsigned int l3sclkstctrl;	/* offset 0x04 */
	unsigned int l4fwclkstctrl;	/* offset 0x08 */
	unsigned int l3clkstctrl;	/* offset 0x0c */
	unsigned int resv1;
	unsigned int cpgmac0clkctrl;	/* offset 0x14 */
	unsigned int lcdclkctrl;	/* offset 0x18 */
	unsigned int usb0clkctrl;	/* offset 0x1C */
	unsigned int resv2;
	unsigned int tptc0clkctrl;	/* offset 0x24 */
	unsigned int emifclkctrl;	/* offset 0x28 */
	unsigned int ocmcramclkctrl;	/* offset 0x2c */
	unsigned int gpmcclkctrl;	/* offset 0x30 */
	unsigned int mcasp0clkctrl;	/* offset 0x34 */
	unsigned int uart5clkctrl;	/* offset 0x38 */
	unsigned int mmc0clkctrl;	/* offset 0x3C */
	unsigned int elmclkctrl;	/* offset 0x40 */
	unsigned int i2c2clkctrl;	/* offset 0x44 */
	unsigned int i2c1clkctrl;	/* offset 0x48 */
	unsigned int spi0clkctrl;	/* offset 0x4C */
	unsigned int spi1clkctrl;	/* offset 0x50 */
	unsigned int resv3[3];
	unsigned int l4lsclkctrl;	/* offset 0x60 */
	unsigned int l4fwclkctrl;	/* offset 0x64 */
	unsigned int mcasp1clkctrl;	/* offset 0x68 */
	unsigned int uart1clkctrl;	/* offset 0x6C */
	unsigned int uart2clkctrl;	/* offset 0x70 */
	unsigned int uart3clkctrl;	/* offset 0x74 */
	unsigned int uart4clkctrl;	/* offset 0x78 */
	unsigned int timer7clkctrl;	/* offset 0x7C */
	unsigned int timer2clkctrl;	/* offset 0x80 */
	unsigned int timer3clkctrl;	/* offset 0x84 */
	unsigned int timer4clkctrl;	/* offset 0x88 */
	unsigned int resv4[8];
	unsigned int gpio1clkctrl;	/* offset 0xAC */
	unsigned int gpio2clkctrl;	/* offset 0xB0 */
	unsigned int gpio3clkctrl;	/* offset 0xB4 */
	unsigned int resv5;
	unsigned int tpccclkctrl;	/* offset 0xBC */
	unsigned int dcan0clkctrl;	/* offset 0xC0 */
	unsigned int dcan1clkctrl;	/* offset 0xC4 */
	unsigned int resv6[2];
	unsigned int emiffwclkctrl;	/* offset 0xD0 */
	unsigned int resv7[2];
	unsigned int l3instrclkctrl;	/* offset 0xDC */
	unsigned int l3clkctrl;		/* Offset 0xE0 */
	unsigned int resv8[4];
	unsigned int mmc1clkctrl;	/* offset 0xF4 */
	unsigned int mmc2clkctrl;	/* offset 0xF8 */
	unsigned int resv9[8];
	unsigned int l4hsclkstctrl;	/* offset 0x11C */
	unsigned int l4hsclkctrl;	/* offset 0x120 */
	unsigned int resv10[8];
	unsigned int cpswclkstctrl;	/* offset 0x144 */
};

/* Encapsulating core pll registers */
struct cm_wkuppll {
	unsigned int wkclkstctrl;	/* offset 0x00 */
	unsigned int wkctrlclkctrl;	/* offset 0x04 */
	unsigned int wkgpio0clkctrl;	/* offset 0x08 */
	unsigned int wkl4wkclkctrl;	/* offset 0x0c */
	unsigned int resv2[4];
	unsigned int idlestdpllmpu;	/* offset 0x20 */
	unsigned int resv3[2];
	unsigned int clkseldpllmpu;	/* offset 0x2c */
	unsigned int resv4[1];
	unsigned int idlestdpllddr;	/* offset 0x34 */
	unsigned int resv5[2];
	unsigned int clkseldpllddr;	/* offset 0x40 */
	unsigned int resv6[4];
	unsigned int clkseldplldisp;	/* offset 0x54 */
	unsigned int resv7[1];
	unsigned int idlestdpllcore;	/* offset 0x5c */
	unsigned int resv8[2];
	unsigned int clkseldpllcore;	/* offset 0x68 */
	unsigned int resv9[1];
	unsigned int idlestdpllper;	/* offset 0x70 */
	unsigned int resv10[2];
	unsigned int clkdcoldodpllper;	/* offset 0x7c */
	unsigned int divm4dpllcore;	/* offset 0x80 */
	unsigned int divm5dpllcore;	/* offset 0x84 */
	unsigned int clkmoddpllmpu;	/* offset 0x88 */
	unsigned int clkmoddpllper;	/* offset 0x8c */
	unsigned int clkmoddpllcore;	/* offset 0x90 */
	unsigned int clkmoddpllddr;	/* offset 0x94 */
	unsigned int clkmoddplldisp;	/* offset 0x98 */
	unsigned int clkseldpllper;	/* offset 0x9c */
	unsigned int divm2dpllddr;	/* offset 0xA0 */
	unsigned int divm2dplldisp;	/* offset 0xA4 */
	unsigned int divm2dpllmpu;	/* offset 0xA8 */
	unsigned int divm2dpllper;	/* offset 0xAC */
	unsigned int resv11[1];
	unsigned int wkup_uart0ctrl;	/* offset 0xB4 */
	unsigned int wkup_i2c0ctrl;	/* offset 0xB8 */
	unsigned int resv12[7];
	unsigned int divm6dpllcore;	/* offset 0xD8 */
};

#define CM_PER				0x44E00000
#define CM_WKUP				0x44E00400

#define PLL_BYPASS_MODE		0x4
#define ST_MN_BYPASS		0x00000100
#define ST_DPLL_CLK		0x00000001
#define CLK_SEL_MASK		0x7ffff
#define CLK_DIV_MASK		0x1f
#define CLK_DIV2_MASK		0x7f
#define CLK_SEL_SHIFT		0x8
#define CLK_MODE_SEL		0x7
#define CLK_MODE_MASK		0xfffffff8
#define CLK_DIV_SEL		0xFFFFFFE0
#define CPGMAC0_IDLE		0x30000
#define DPLL_CLKDCOLDO_GATE_CTRL        0x300

#define V_OSCK				24000000  /* Clock output from T2 */
#define OSC	(V_OSCK/1000000)

#define DDRPLL_M	266
#define DDRPLL_N	(OSC-1)
#define DDRPLL_M2	1

#endif