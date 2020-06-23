#include "ddr_init.h"
#include <types.h>
#include <device/mmio.h>
#include <delay.h>
#include <console/console.h>

static struct vtp_reg *vtpreg[2] = {
				(struct vtp_reg *)VTP0_CTRL_ADDR,
				(struct vtp_reg *)VTP1_CTRL_ADDR};

/**
 * Base address for EMIF instances
 */
static struct emif_reg_struct *emif_reg[2] = {
				(struct emif_reg_struct *)EMIF4_0_CFG_BASE,
				(struct emif_reg_struct *)EMIF4_1_CFG_BASE};

/**
 * Base addresses for DDR PHY cmd/data regs
 */
static struct ddr_cmd_regs *ddr_cmd_reg[2] = {
				(struct ddr_cmd_regs *)DDR_PHY_CMD_ADDR,
				(struct ddr_cmd_regs *)DDR_PHY_CMD_ADDR2};

static struct ddr_data_regs *ddr_data_reg[2] = {
				(struct ddr_data_regs *)DDR_PHY_DATA_ADDR,
				(struct ddr_data_regs *)DDR_PHY_DATA_ADDR2};

/**
 * Base address for ddr io control instances
 */
static struct ddr_cmdtctrl *ioctrl_reg = {
			(struct ddr_cmdtctrl *)DDR_CONTROL_BASE_ADDR};

struct ctrl_stat *cstat = (struct ctrl_stat *)CTRL_BASE;

static struct ddr_ctrl *ddrctrl = (struct ddr_ctrl *)DDR_CTRL_ADDR;

static void writel(uint32_t val, const void *addr)
{
    write32((void*)addr, val);
}

static uint32_t readl(const void* addr)
{
    return read32(addr);
}

static void config_vtp(int nr)
{
	writel(readl(&vtpreg[nr]->vtp0ctrlreg) | VTP_CTRL_ENABLE,
			&vtpreg[nr]->vtp0ctrlreg);
	writel(readl(&vtpreg[nr]->vtp0ctrlreg) & (~VTP_CTRL_START_EN),
			&vtpreg[nr]->vtp0ctrlreg);
	writel(readl(&vtpreg[nr]->vtp0ctrlreg) | VTP_CTRL_START_EN,
			&vtpreg[nr]->vtp0ctrlreg);

	/* Poll for READY */
	while ((readl(&vtpreg[nr]->vtp0ctrlreg) & VTP_CTRL_READY) !=
			VTP_CTRL_READY)
		;
}

/**
 * Configure SDRAM
 */
static void config_sdram(const struct emif_regs *regs, int nr)
{
	if (regs->zq_config) {
		writel(regs->zq_config, &emif_reg[nr]->emif_zq_config);
		writel(regs->sdram_config, &cstat->secure_emif_sdram_config);
		writel(regs->sdram_config, &emif_reg[nr]->emif_sdram_config);

		/* Trigger initialization */
		writel(0x00003100, &emif_reg[nr]->emif_sdram_ref_ctrl);
		/* Wait 1ms because of L3 timeout error */
		udelay(1000);

		/* Write proper sdram_ref_cref_ctrl value */
		writel(regs->ref_ctrl, &emif_reg[nr]->emif_sdram_ref_ctrl);
		writel(regs->ref_ctrl, &emif_reg[nr]->emif_sdram_ref_ctrl_shdw);
	}
	writel(regs->ref_ctrl, &emif_reg[nr]->emif_sdram_ref_ctrl);
	writel(regs->ref_ctrl, &emif_reg[nr]->emif_sdram_ref_ctrl_shdw);
	writel(regs->sdram_config, &emif_reg[nr]->emif_sdram_config);

	/* Write REG_COS_COUNT_1, REG_COS_COUNT_2, and REG_PR_OLD_COUNT. */
	if (regs->ocp_config)
		writel(regs->ocp_config, &emif_reg[nr]->emif_l3_config);
}

/**
 * Configure DDR DATA registers
 */
static void config_ddr_data(const struct ddr_data *data, int nr)
{
	int i;

	if (!data)
		return;

	for (i = 0; i < DDR_DATA_REGS_NR; i++) {
		writel(data->datardsratio0,
			&(ddr_data_reg[nr]+i)->dt0rdsratio0);
		writel(data->datawdsratio0,
			&(ddr_data_reg[nr]+i)->dt0wdsratio0);
		writel(data->datawiratio0,
			&(ddr_data_reg[nr]+i)->dt0wiratio0);
		writel(data->datagiratio0,
			&(ddr_data_reg[nr]+i)->dt0giratio0);
		writel(data->datafwsratio0,
			&(ddr_data_reg[nr]+i)->dt0fwsratio0);
		writel(data->datawrsratio0,
			&(ddr_data_reg[nr]+i)->dt0wrsratio0);
	}
}

static void config_io_ctrl(const struct ctrl_ioregs *ioregs)
{
	if (!ioregs)
		return;

	writel(ioregs->cm0ioctl, &ioctrl_reg->cm0ioctl);
	writel(ioregs->cm1ioctl, &ioctrl_reg->cm1ioctl);
	writel(ioregs->cm2ioctl, &ioctrl_reg->cm2ioctl);
	writel(ioregs->dt0ioctl, &ioctrl_reg->dt0ioctl);
	writel(ioregs->dt1ioctl, &ioctrl_reg->dt1ioctl);
}


/**
 * Configure DDR CMD control registers
 */
static void config_cmd_ctrl(const struct cmd_control *cmd, int nr)
{
	if (!cmd)
		return;

	writel(cmd->cmd0csratio, &ddr_cmd_reg[nr]->cm0csratio);
	writel(cmd->cmd0iclkout, &ddr_cmd_reg[nr]->cm0iclkout);

	writel(cmd->cmd1csratio, &ddr_cmd_reg[nr]->cm1csratio);
	writel(cmd->cmd1iclkout, &ddr_cmd_reg[nr]->cm1iclkout);

	writel(cmd->cmd2csratio, &ddr_cmd_reg[nr]->cm2csratio);
	writel(cmd->cmd2iclkout, &ddr_cmd_reg[nr]->cm2iclkout);
}

static inline u32 get_emif_rev(u32 base)
{
	struct emif_reg_struct *emif = (struct emif_reg_struct *)base;

	return (readl(&emif->emif_mod_id_rev) & EMIF_REG_MAJOR_REVISION_MASK)
		>> EMIF_REG_MAJOR_REVISION_SHIFT;
}

/*
 * Get SDRAM type connected to EMIF.
 * Assuming similar SDRAM parts are connected to both EMIF's
 * which is typically the case. So it is sufficient to get
 * SDRAM type from EMIF1.
 */
static inline u32 emif_sdram_type(u32 sdram_config)
{
	return (sdram_config & EMIF_REG_SDRAM_TYPE_MASK)
	       >> EMIF_REG_SDRAM_TYPE_SHIFT;
}

/*
 * Configure EXT PHY registers for software leveling
 */
static void ext_phy_settings_swlvl(const struct emif_regs *regs, int nr)
{
	uint32_t *ext_phy_ctrl_base = 0;
	u32 *emif_ext_phy_ctrl_base = 0;
	u32 i = 0;

	ext_phy_ctrl_base = (u32 *)&(regs->emif_ddr_ext_phy_ctrl_1);
	emif_ext_phy_ctrl_base =
			(u32 *)&(emif_reg[nr]->emif_ddr_ext_phy_ctrl_1);

	/* Configure external phy control timing registers */
	for (i = 0; i < EMIF_EXT_PHY_CTRL_TIMING_REG; i++) {
		writel(*ext_phy_ctrl_base, emif_ext_phy_ctrl_base++);
		/* Update shadow registers */
		writel(*ext_phy_ctrl_base++, emif_ext_phy_ctrl_base++);
	}
}

/*
 * Configure EXT PHY registers for hardware leveling
 */
static void ext_phy_settings_hwlvl(const struct emif_regs *regs, int nr)
{
	/*
	 * Enable hardware leveling on the EMIF.  For details about these
	 * magic values please see the EMIF registers section of the TRM.
	 */
	if (regs->emif_ddr_phy_ctlr_1 & 0x00040000) {
		/* PHY_INVERT_CLKOUT = 1 */
		writel(0x00040100, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_1);
		writel(0x00040100, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_1_shdw);
	} else {
		/* PHY_INVERT_CLKOUT = 0 */
		writel(0x08020080, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_1);
		writel(0x08020080, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_1_shdw);
	}

	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_22);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_22_shdw);
	writel(0x00600020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_23);
	writel(0x00600020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_23_shdw);
	writel(0x40010080, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_24);
	writel(0x40010080, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_24_shdw);
	writel(0x08102040, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_25);
	writel(0x08102040, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_25_shdw);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_26);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_26_shdw);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_27);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_27_shdw);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_28);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_28_shdw);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_29);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_29_shdw);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_30);
	writel(0x00200020, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_30_shdw);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_31);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_31_shdw);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_32);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_32_shdw);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_33);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_33_shdw);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_34);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_34_shdw);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_35);
	writel(0x00000000, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_35_shdw);
	writel(0x00000077, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_36);
	writel(0x00000077, &emif_reg[nr]->emif_ddr_ext_phy_ctrl_36_shdw);

	/*
	 * Sequence to ensure that the PHY is again in a known state after
	 * hardware leveling.
	 */
	writel(0x2011, &emif_reg[nr]->emif_iodft_tlgc);
	writel(0x2411, &emif_reg[nr]->emif_iodft_tlgc);
	writel(0x2011, &emif_reg[nr]->emif_iodft_tlgc);
}


/**
 * Configure DDR PHY
 */
static void config_ddr_phy(const struct emif_regs *regs, int nr)
{
	/*
	 * Disable initialization and refreshes for now until we finish
	 * programming EMIF regs and set time between rising edge of
	 * DDR_RESET to rising edge of DDR_CKE to > 500us per memory spec.
	 * We currently hardcode a value based on a max expected frequency
	 * of 400MHz.
	 */
	writel(EMIF_REG_INITREF_DIS_MASK | 0x3100,
		&emif_reg[nr]->emif_sdram_ref_ctrl);

	writel(regs->emif_ddr_phy_ctlr_1,
		&emif_reg[nr]->emif_ddr_phy_ctrl_1);
	writel(regs->emif_ddr_phy_ctlr_1,
		&emif_reg[nr]->emif_ddr_phy_ctrl_1_shdw);

	if (get_emif_rev((u32)emif_reg[nr]) == EMIF_4D5) {
		if (emif_sdram_type(regs->sdram_config) == EMIF_SDRAM_TYPE_DDR3)
			ext_phy_settings_hwlvl(regs, nr);
		else
			ext_phy_settings_swlvl(regs, nr);
	}
}

/**
 * Set SDRAM timings
 */
static void set_sdram_timings(const struct emif_regs *regs, int nr)
{
	writel(regs->sdram_tim1, &emif_reg[nr]->emif_sdram_tim_1);
	writel(regs->sdram_tim1, &emif_reg[nr]->emif_sdram_tim_1_shdw);
	writel(regs->sdram_tim2, &emif_reg[nr]->emif_sdram_tim_2);
	writel(regs->sdram_tim2, &emif_reg[nr]->emif_sdram_tim_2_shdw);
	writel(regs->sdram_tim3, &emif_reg[nr]->emif_sdram_tim_3);
	writel(regs->sdram_tim3, &emif_reg[nr]->emif_sdram_tim_3_shdw);
}
const struct cm_wkuppll *cmwkup = (struct cm_wkuppll *)CM_WKUP;

static void ddr_pll_config(unsigned int ddrpll_m)
{
	u32 clkmode, clksel, div_m2;

	clkmode = readl(&cmwkup->clkmoddpllddr);
	clksel = readl(&cmwkup->clkseldpllddr);
	div_m2 = readl(&cmwkup->divm2dpllddr);

	/* Set the PLL to bypass Mode */
	clkmode = (clkmode & CLK_MODE_MASK) | PLL_BYPASS_MODE;
	writel(clkmode, &cmwkup->clkmoddpllddr);

	/* Wait till bypass mode is enabled */
	while ((readl(&cmwkup->idlestdpllddr) & ST_MN_BYPASS)
				!= ST_MN_BYPASS)
		;

	clksel = clksel & (~CLK_SEL_MASK);
	clksel = clksel | ((ddrpll_m << CLK_SEL_SHIFT) | DDRPLL_N);
	writel(clksel, &cmwkup->clkseldpllddr);

	div_m2 = div_m2 & CLK_DIV_SEL;
	div_m2 = div_m2 | DDRPLL_M2;
	writel(div_m2, &cmwkup->divm2dpllddr);

	clkmode = (clkmode & CLK_MODE_MASK) | CLK_MODE_SEL;
	writel(clkmode, &cmwkup->clkmoddpllddr);

	/* Wait till dpll is locked */
	while ((readl(&cmwkup->idlestdpllddr) & ST_DPLL_CLK) != ST_DPLL_CLK)
		;
}

const struct cm_perpll *cmper = (struct cm_perpll *)CM_PER;

static void enable_emif_clocks(void)
{
	/* Enable the  EMIF_FW Functional clock */
	writel(PRCM_MOD_EN, &cmper->emiffwclkctrl);
	/* Enable EMIF0 Clock */
	writel(PRCM_MOD_EN, &cmper->emifclkctrl);
	/* Poll if module is functional */
	while ((readl(&cmper->emifclkctrl)) != PRCM_MOD_EN)
		;
}



void config_ddr(unsigned int pll, const struct ctrl_ioregs *ioregs,
		const struct ddr_data *data, const struct cmd_control *ctrl,
		const struct emif_regs *regs, int nr)
{
    enable_emif_clocks();

    ddr_pll_config(pll);

	config_vtp(nr);

	config_cmd_ctrl(ctrl, nr);

	config_ddr_data(data, nr);

	config_io_ctrl(ioregs);
    printk(BIOS_DEBUG, "%d\n", __LINE__);

	/* Set CKE to be controlled by EMIF/DDR PHY */
	writel(DDR_CKE_CTRL_NORMAL, &ddrctrl->ddrckectrl);
    printk(BIOS_DEBUG, "%d\n", __LINE__);

	/* Program EMIF instance */
	config_ddr_phy(regs, nr);
    printk(BIOS_DEBUG, "%d\n", __LINE__);
	set_sdram_timings(regs, nr);
    printk(BIOS_DEBUG, "%d\n", __LINE__);

	config_sdram(regs, nr);
}
