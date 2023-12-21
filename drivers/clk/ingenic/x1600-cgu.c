// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Ingenic X1600 SoC CGU driver
 *
 * Copyright (c) 2013-2015 Imagination Technologies
 * Author: Paul Burton <paul.burton@mips.com>
 * Copyright (c) 2020 周琰杰 (Zhou Yanjie) <zhouyanjie@wanyeetech.com>
 * Copyright (c) 2023 Paul Boddie <paul@boddie.org.uk>
 */

#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/of.h>

#include <dt-bindings/clock/ingenic,x1600-cgu.h>

#include "cgu.h"
#include "pm.h"

/* CGU register offsets */
#define CGU_REG_CLOCKCONTROL	0x00
#define CGU_REG_LCR		0x04
#define CGU_REG_APLL		0x10
#define CGU_REG_MPLL		0x14
#define CGU_REG_EPLL		0x18
#define CGU_REG_CLKGR0		0x20
#define CGU_REG_OPCR		0x24
#define CGU_REG_CLKGR1		0x28
#define CGU_REG_DDRCDR		0x2c
#define CGU_REG_CPSPR		0x34
#define CGU_REG_CPSPPR		0x38
#define CGU_REG_USBPCR		0x3c
#define CGU_REG_USBRDT		0x40
#define CGU_REG_USBVBFIL	0x44
#define CGU_REG_USBPCR1		0x48
#define CGU_REG_MACCDR		0x54
#define CGU_REG_SSICDR		0x5c
#define CGU_REG_I2S0CDR		0x60
#define CGU_REG_LPCDR		0x64
#define CGU_REG_MSC0CDR		0x68
#define CGU_REG_PWMCDR		0x6c
#define CGU_REG_I2SCDR1		0x70
#define CGU_REG_SFCCDR		0x74
#define CGU_REG_CIMCDR		0x78
#define CGU_REG_I2S1CDR		0x7c
#define CGU_REG_I2S1CDR1	0x80
#define CGU_REG_APLLFRAC	0x84
#define CGU_REG_MPLLFRAC	0x88
#define CGU_REG_EPLLFRAC	0x8c
#define CGU_REG_CAN0CDR		0xa0
#define CGU_REG_MSC1CDR		0xa4
#define CGU_REG_CAN1CDR		0xa8
#define CGU_REG_CDBUSCDR	0xac
#define CGU_REG_CMP_INTR	0xb0
#define CGU_REG_CMP_INTRE	0xb4
#define CGU_REG_CMP_SFTINT	0xbc
#define CGU_REG_DRCG		0xd0
#define CGU_REG_CLOCKSTATUS	0xd4
#define CGU_REG_MPHY0C		0xe4

/* bits within the OPCR register */
#define OPCR_SPENDN0		BIT(7)

static struct ingenic_cgu *cgu;

static int x1600_otg_phy_enable(struct clk_hw *hw)
{
	void __iomem *reg_opcr		= cgu->base + CGU_REG_OPCR;

	writel(readl(reg_opcr) | OPCR_SPENDN0, reg_opcr);

	return 0;
}

static void x1600_otg_phy_disable(struct clk_hw *hw)
{
	void __iomem *reg_opcr		= cgu->base + CGU_REG_OPCR;

	writel(readl(reg_opcr) & ~OPCR_SPENDN0, reg_opcr);
}

static int x1600_otg_phy_is_enabled(struct clk_hw *hw)
{
	void __iomem *reg_opcr		= cgu->base + CGU_REG_OPCR;

	return (readl(reg_opcr) & OPCR_SPENDN0);
}

static const struct clk_ops x1600_otg_phy_ops = {
	.enable		= x1600_otg_phy_enable,
	.disable	= x1600_otg_phy_disable,
	.is_enabled	= x1600_otg_phy_is_enabled,
};

static const struct ingenic_cgu_clk_info x1600_cgu_clocks[] = {

	/* External clocks */

	[X1600_CLK_EXCLK] = { "ext", CGU_CLK_EXT },
	[X1600_CLK_RTCLK] = { "rtc", CGU_CLK_EXT },
	[X1600_CLK_12M] = { "clk12m", CGU_CLK_EXT },

	/* PLLs */

#define DEF_PLL(name) { \
	.reg = CGU_REG_ ## name, \
	.rate_multiplier = 1, \
	.m_shift = 20, \
	.m_bits = 12, \
	.m_offset = 0, \
	.n_shift = 14, \
	.n_bits = 6, \
	.n_offset = 0, \
	.od_shift = 8, \
	.od_bits = 3, \
	.od_max = 8, \
	.od1_shift = 11, \
	.od1_bits = 3, \
	.od1_max = 8, \
	.od_encoding = 0, \
	.stable_bit = 2, \
	.enable_bit = 0, \
}

	[X1600_CLK_APLL] = {
		"apll", CGU_CLK_PLL,
		.parents = { X1600_CLK_EXCLK, -1, -1, -1 },
		.pll = DEF_PLL(APLL),
	},

	[X1600_CLK_MPLL] = {
		"mpll", CGU_CLK_PLL,
		.parents = { X1600_CLK_EXCLK, -1, -1, -1 },
		.pll = DEF_PLL(MPLL),
	},

	[X1600_CLK_EPLL] = {
		"epll", CGU_CLK_PLL,
		.parents = { X1600_CLK_EXCLK, -1, -1, -1 },
		.pll = DEF_PLL(EPLL),
	},

#undef DEF_PLL

	/* Muxes & dividers */

	[X1600_CLK_SCLKA] = {
		"sclk_a", CGU_CLK_MUX,
		.parents = { -1, X1600_CLK_EXCLK, X1600_CLK_APLL, -1 },
		.mux = { CGU_REG_CLOCKCONTROL, 30, 2 },
	},

	[X1600_CLK_CPUMUX] = {
		"cpumux", CGU_CLK_MUX,
		.parents = { -1, X1600_CLK_SCLKA, X1600_CLK_MPLL, -1 },
		.mux = { CGU_REG_CLOCKCONTROL, 28, 2 },
	},

	[X1600_CLK_CPU] = {
		"cpu", CGU_CLK_DIV,
		/*
		 * Disabling the CPU clock or any parent clocks will hang the
		 * system; mark it critical.
		 */
		.flags = CLK_IS_CRITICAL,
		.parents = { X1600_CLK_CPUMUX, -1, -1, -1 },
		.div = { CGU_REG_CLOCKCONTROL, 0, 1, 4, 22, -1, -1 },
	},

	[X1600_CLK_L2CACHE] = {
		"l2cache", CGU_CLK_DIV,
		/*
		 * The L2 cache clock is critical if caches are enabled and
		 * disabling it or any parent clocks will hang the system.
		 */
		.flags = CLK_IS_CRITICAL,
		.parents = { X1600_CLK_CPUMUX, -1, -1, -1 },
		.div = { CGU_REG_CLOCKCONTROL, 4, 1, 4, -1, -1, -1 },
	},

	[X1600_CLK_AHB0] = {
		"ahb0", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { -1, X1600_CLK_SCLKA, X1600_CLK_MPLL, -1 },
		.mux = { CGU_REG_CLOCKCONTROL, 26, 2 },
		.div = { CGU_REG_CLOCKCONTROL, 8, 1, 4, 21, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 29 },
	},

	[X1600_CLK_AHB2PMUX] = {
		"ahb2_apb_mux", CGU_CLK_MUX,
		.parents = { -1, X1600_CLK_SCLKA, X1600_CLK_MPLL, -1 },
		.mux = { CGU_REG_CLOCKCONTROL, 24, 2 },
	},

	[X1600_CLK_AHB2] = {
		"ahb2", CGU_CLK_DIV,
		.parents = { X1600_CLK_AHB2PMUX, -1, -1, -1 },
		.div = { CGU_REG_CLOCKCONTROL, 12, 1, 4, 20, -1, -1 },
	},

	[X1600_CLK_PCLK] = {
		"pclk", CGU_CLK_DIV,
		.parents = { X1600_CLK_AHB2PMUX, -1, -1, -1 },
		.div = { CGU_REG_CLOCKCONTROL, 16, 1, 4, 20, -1, -1 },
	},

	[X1600_CLK_DDR] = {
		"ddr", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		/*
		 * Disabling DDR clock or its parents will render DRAM
		 * inaccessible; mark it critical.
		 */
		.flags = CLK_IS_CRITICAL,
		.parents = { -1, X1600_CLK_SCLKA, X1600_CLK_MPLL, -1 },
		.mux = { CGU_REG_DDRCDR, 30, 2 },
		.div = { CGU_REG_DDRCDR, 0, 1, 4, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 31 },
	},

	[X1600_CLK_I2S0] = {
		"i2s0", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { X1600_CLK_SCLKA, X1600_CLK_EPLL, -1, -1 },
		.mux = { CGU_REG_I2S0CDR, 30, 1 },
		.mndiv = { CGU_REG_I2S0CDR, 20, 1, 9, 0, 1, 20, -1, -1, -1 },
		.gate = { CGU_REG_I2S0CDR, 29 },
	},

	[X1600_CLK_I2S1] = {
		"i2s1", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { X1600_CLK_SCLKA, X1600_CLK_EPLL, -1, -1 },
		.mux = { CGU_REG_I2S1CDR, 30, 1 },
		.mndiv = { CGU_REG_I2S1CDR, 20, 1, 9, 0, 1, 20, -1, -1, -1 },
		.gate = { CGU_REG_I2S1CDR, 29 },
	},

	[X1600_CLK_LCDPIXCLK] = {
		"lcd0pixclk", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { X1600_CLK_SCLKA, X1600_CLK_MPLL,
			     X1600_CLK_EPLL, -1 },
		.mux = { CGU_REG_LPCDR, 30, 2 },
		.div = { CGU_REG_LPCDR, 0, 1, 8, 28, 27, 26 },
		.gate = { CGU_REG_CLKGR0, 23 },
	},

	[X1600_CLK_MAC] = {
		"mac", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { X1600_CLK_SCLKA, X1600_CLK_MPLL,
			     X1600_CLK_EPLL, -1 },
		.mux = { CGU_REG_MACCDR, 30, 2 },
		.div = { CGU_REG_MACCDR, 0, 1, 8, 28, 27, 26 },
		.gate = { CGU_REG_CLKGR1, 23 },
	},

	[X1600_CLK_MSC0] = {
		"msc0", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { X1600_CLK_SCLKA, X1600_CLK_MPLL,
			     X1600_CLK_EPLL, -1 },
		.mux = { CGU_REG_MSC0CDR, 30, 2 },
		.div = { CGU_REG_MSC0CDR, 0, 2, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 4 },
	},

	[X1600_CLK_MSC1] = {
		"msc1", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { X1600_CLK_SCLKA, X1600_CLK_MPLL,
			     X1600_CLK_EPLL, -1 },
		.mux = { CGU_REG_MSC1CDR, 30, 2 },
		.div = { CGU_REG_MSC1CDR, 0, 2, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 5 },
	},

	[X1600_CLK_SSI0] = {
		"ssi", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { X1600_CLK_SCLKA, X1600_CLK_MPLL,
			     X1600_CLK_EPLL, -1 },
		.mux = { CGU_REG_SSICDR, 30, 2 },
		.div = { CGU_REG_SSICDR, 0, 1, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 19 },
	},

	[X1600_CLK_CIMMCLK] = {
		"cim_mclk", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { X1600_CLK_SCLKA, X1600_CLK_MPLL,
			     X1600_CLK_EPLL, -1 },
		.mux = { CGU_REG_CIMCDR, 30, 2 },
		.div = { CGU_REG_CIMCDR, 0, 1, 8, 30, 29, 28 },
		.gate = { CGU_REG_CLKGR0, 22 },
	},

	[X1600_CLK_EXCLK_DIV512] = {
		"exclk_div512", CGU_CLK_FIXDIV,
		.parents = { X1600_CLK_EXCLK },
		.fixdiv = { 512 },
	},

	[X1600_CLK_RTC] = {
		"rtc_ercs", CGU_CLK_MUX | CGU_CLK_GATE,
		.parents = { X1600_CLK_EXCLK_DIV512, X1600_CLK_RTCLK },
		.mux = { CGU_REG_OPCR, 2, 1},
	},

	/* Custom (SoC-specific) OTG PHY */

	[X1600_CLK_OTGPHY] = {
		"otg_phy", CGU_CLK_CUSTOM,
		.parents = { X1600_CLK_12M },
		.custom = { &x1600_otg_phy_ops },
	},

	/* Gate-only clocks */

	[X1600_CLK_NEMC] = {
		"nemc", CGU_CLK_GATE,
		.parents = { X1600_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 0 },
	},

	[X1600_CLK_OTG0] = {
		"otg0", CGU_CLK_GATE,
		.parents = { X1600_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 3 },
	},

	[X1600_CLK_SMB0] = {
		"smb0", CGU_CLK_GATE,
		.parents = { X1600_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 7 },
	},

	[X1600_CLK_SMB1] = {
		"smb1", CGU_CLK_GATE,
		.parents = { X1600_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 8 },
	},

	[X1600_CLK_AIC] = {
		"aic", CGU_CLK_GATE,
		.parents = { X1600_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 11 },
	},

	[X1600_CLK_SADC] = {
		"sadc", CGU_CLK_GATE,
		.parents = { X1600_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 14 },
	},

	[X1600_CLK_UART0] = {
		"uart0", CGU_CLK_GATE,
		.parents = { X1600_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 14 },
	},

	[X1600_CLK_UART1] = {
		"uart1", CGU_CLK_GATE,
		.parents = { X1600_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 15 },
	},

	[X1600_CLK_UART2] = {
		"uart2", CGU_CLK_GATE,
		.parents = { X1600_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 16 },
	},

	[X1600_CLK_UART3] = {
		"uart3", CGU_CLK_GATE,
		.parents = { X1600_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 16 },
	},

	[X1600_CLK_PDMA] = {
		"pdma", CGU_CLK_GATE,
		.parents = { X1600_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 21 },
	},

	[X1600_CLK_AES] = {
		"des", CGU_CLK_GATE,
		.parents = { X1600_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 24 },
	},

};

static void __init x1600_cgu_init(struct device_node *np)
{
	int retval;

	cgu = ingenic_cgu_new(x1600_cgu_clocks,
			      ARRAY_SIZE(x1600_cgu_clocks), np);
	if (!cgu) {
		pr_err("%s: failed to initialise CGU\n", __func__);
		return;
	}

	retval = ingenic_cgu_register_clocks(cgu);
	if (retval) {
		pr_err("%s: failed to register CGU Clocks\n", __func__);
		return;
	}

	ingenic_cgu_register_syscore_ops(cgu);
}
CLK_OF_DECLARE_DRIVER(x1600_cgu, "ingenic,x1600-cgu", x1600_cgu_init);
