// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Ingenic JZ4730 SoC CGU driver
 *
 * Copyright (c) 2015 Imagination Technologies
 * Author: Paul Burton <paul.burton@mips.com>
 *
 * Copyright (c) 2017, 2019, 2020, 2021 Paul Boddie <paul@boddie.org.uk>
 */

#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>

#include <dt-bindings/clock/jz4730-cgu.h>

#include "cgu.h"
#include "pm.h"

/* CGU register offsets */
#define CGU_REG_CFCR		0x00 /* CPCCR in jz4740 */
#define CGU_REG_LPCR		0x04 /* LCR in jz4740 */
#define CGU_REG_PLCR1		0x10 /* CPPCR in jz4740 */
#define CGU_REG_OCR		0x1c
#define CGU_REG_MSCR		0x20 /* CLKGR in jz4740 */
#define CGU_REG_CFCR2		0x60 /* LPCDR in jz4740 */

static struct ingenic_cgu *cgu;

static const s8 pll_od_encoding[4] = {
	0x0, 0x1, -1, 0x3,
};

static const s8 jz4730_cgu_cfcr_div_table[] = {
	1, 2, 3, 4, 6, 8, 12, 16, 24, 32
};

static const struct ingenic_cgu_clk_info jz4730_cgu_clocks[] = {
	/* External clocks */

	[JZ4730_CLK_EXT] = { "ext", CGU_CLK_EXT },
	[JZ4730_CLK_RTC] = { "rtc", CGU_CLK_EXT },
	[JZ4730_CLK_MSC16M] = { "msc16m", CGU_CLK_EXT },
	[JZ4730_CLK_MSC24M] = { "msc24m", CGU_CLK_EXT },
	[JZ4730_CLK_USB48M] = { "usb48m", CGU_CLK_EXT },

	[JZ4730_CLK_PLL] = {
		"pll", CGU_CLK_PLL,
		.parents = { JZ4730_CLK_EXT, -1, -1, -1 },
		.pll = {
			.reg = CGU_REG_PLCR1,
			.rate_multiplier = 1,
			.m_shift = 23,
			.m_bits = 9,
			.m_offset = 2,
			.n_shift = 18,
			.n_bits = 5,
			.n_offset = 2,
			.od_shift = 16,
			.od_bits = 2,
			.od_max = 4,
			.od_encoding = pll_od_encoding,
			.stable_bit = 10,
			.bypass_reg = CGU_REG_PLCR1,
			.bypass_bit = 9,
			.enable_bit = 8,
		},
	},

	/* Muxes & dividers */

	[JZ4730_CLK_PLL_HALF] = {
		"pll half", CGU_CLK_FIXDIV,
		.parents = { JZ4730_CLK_PLL, -1, -1, -1 },
		.div = { 2 },
	},

	[JZ4730_CLK_CCLK_PLL] = {
		"cclkdiv", CGU_CLK_DIV,
		.parents = { JZ4730_CLK_PLL, -1, -1, -1 },
		.div = {
			CGU_REG_CFCR, 0, 1, 4, 20, -1, -1, -1,
			jz4730_cgu_cfcr_div_table,
		},
	},

	[JZ4730_CLK_CCLK] = {
		"cclk", CGU_CLK_MUX,
		.parents = { JZ4730_CLK_EXT, JZ4730_CLK_CCLK_PLL, -1, -1 },
		.mux = { CGU_REG_PLCR1, 8, 1 },
	},

	[JZ4730_CLK_HCLK_PLL] = {
		"hclkdiv", CGU_CLK_DIV,
		.parents = { JZ4730_CLK_PLL, -1, -1, -1 },
		.div = {
			CGU_REG_CFCR, 4, 1, 4, 20, -1, -1, -1,
			jz4730_cgu_cfcr_div_table,
		},
	},

	[JZ4730_CLK_HCLK] = {
		"hclk", CGU_CLK_MUX,
		.parents = { JZ4730_CLK_EXT, JZ4730_CLK_HCLK_PLL, -1, -1 },
		.mux = { CGU_REG_PLCR1, 8, 1 },
	},

	[JZ4730_CLK_PCLK_PLL] = {
		"pclkdiv", CGU_CLK_DIV,
		.parents = { JZ4730_CLK_PLL, -1, -1, -1 },
		.div = {
			CGU_REG_CFCR, 8, 1, 4, 20, -1, -1, -1,
			jz4730_cgu_cfcr_div_table,
		},
	},

	[JZ4730_CLK_PCLK] = {
		"pclk", CGU_CLK_MUX,
		.parents = { JZ4730_CLK_EXT, JZ4730_CLK_PCLK_PLL, -1, -1 },
		.mux = { CGU_REG_PLCR1, 8, 1 },
	},

	[JZ4730_CLK_MCLK_PLL] = {
		"mclkdiv", CGU_CLK_DIV,
		.parents = { JZ4730_CLK_PLL, -1, -1, -1 },
		.div = {
			CGU_REG_CFCR, 16, 1, 4, 20, -1, -1, -1,
			jz4730_cgu_cfcr_div_table,
		},
	},

	[JZ4730_CLK_MCLK] = {
		"mclk", CGU_CLK_MUX,
		.parents = { JZ4730_CLK_EXT, JZ4730_CLK_MCLK_PLL, -1, -1 },
		.mux = { CGU_REG_PLCR1, 8, 1 },
	},

	[JZ4730_CLK_LCD_PLL] = {
		"lcddiv", CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { JZ4730_CLK_PLL, -1, -1, -1 },
		.div = {
			CGU_REG_CFCR, 12, 1, 4, 20, -1, -1, -1,
			jz4730_cgu_cfcr_div_table,
		},
		.gate = { CGU_REG_MSCR, 7 },
	},

	[JZ4730_CLK_LCD] = {
		"lcd", CGU_CLK_MUX,
		.parents = { JZ4730_CLK_EXT, JZ4730_CLK_LCD_PLL, -1, -1 },
		.mux = { CGU_REG_PLCR1, 8, 1 },
	},

	[JZ4730_CLK_LCD_PCLK] = {
		"lcd_pclk", CGU_CLK_DIV,
		.parents = { JZ4730_CLK_PLL, -1, -1, -1 },
		.div = { CGU_REG_CFCR2, 0, 1, 9, -1, -1, -1, -1, },
	},

	[JZ4730_CLK_I2S] = {
		"i2s", CGU_CLK_MUX | CGU_CLK_GATE,
		.parents = { JZ4730_CLK_PLL, JZ4730_CLK_PLL_HALF, -1, -1 },
		.mux = { CGU_REG_CFCR, 29, 1 },
		.gate = { CGU_REG_MSCR, 9 },
	},

	[JZ4730_CLK_SPI] = {
		"spi", CGU_CLK_MUX | CGU_CLK_GATE,
		.parents = { JZ4730_CLK_PLL, JZ4730_CLK_PLL_HALF, -1, -1 },
		.mux = { CGU_REG_CFCR, 31, 1 },
		.gate = { CGU_REG_MSCR, 12 },
	},

	[JZ4730_CLK_MMC] = {
		"mmc", CGU_CLK_MUX | CGU_CLK_GATE,
		.parents = { JZ4730_CLK_MSC16M, JZ4730_CLK_MSC24M, -1, -1 },
		.mux = { CGU_REG_CFCR, 24, 1 },
		.gate = { CGU_REG_MSCR, 13 },
	},

	[JZ4730_CLK_UHC_IN] = {
		"uhcdiv", CGU_CLK_DIV,
		.parents = { JZ4730_CLK_PLL, -1, -1, -1 },
		.div = { CGU_REG_CFCR, 25, 1, 3, 20, -1, -1, -1, },
	},

	[JZ4730_CLK_UHC] = {
		"uhc", CGU_CLK_MUX,
		.parents = { JZ4730_CLK_UHC_IN, JZ4730_CLK_USB48M, -1, -1 },
		.mux = { CGU_REG_CFCR, 28, 1 },
	},

	[JZ4730_CLK_EXT_128] = {
		"ext_div128", CGU_CLK_FIXDIV,
		.parents = { JZ4730_CLK_EXT, -1, -1, -1 },
		.div = { 128 },
	},

	[JZ4730_CLK_WDT] = {
		"wdt", CGU_CLK_MUX,
		.parents = { JZ4730_CLK_EXT_128, JZ4730_CLK_RTC, -1, -1 },
		.mux = { CGU_REG_OCR, 8, 1 },
	},

	/* Gate-only clocks */

	[JZ4730_CLK_UART0] = {
		"uart0", CGU_CLK_GATE,
		.parents = { JZ4730_CLK_EXT, -1, -1, -1 },
		.gate = { CGU_REG_MSCR, 0 },
	},

	[JZ4730_CLK_UART1] = {
		"uart1", CGU_CLK_GATE,
		.parents = { JZ4730_CLK_EXT, -1, -1, -1 },
		.gate = { CGU_REG_MSCR, 1 },
	},

	[JZ4730_CLK_UART2] = {
		"uart2", CGU_CLK_GATE,
		.parents = { JZ4730_CLK_EXT, -1, -1, -1 },
		.gate = { CGU_REG_MSCR, 2 },
	},

	[JZ4730_CLK_UART3] = {
		"uart3", CGU_CLK_GATE,
		.parents = { JZ4730_CLK_EXT, -1, -1, -1 },
		.gate = { CGU_REG_MSCR, 20 },
	},

	[JZ4730_CLK_DMA] = {
		"dma", CGU_CLK_GATE,
		.parents = { JZ4730_CLK_HCLK, -1, -1, -1 },
		.gate = { CGU_REG_MSCR, 5 },
	},

	[JZ4730_CLK_I2C] = {
		"i2c", CGU_CLK_GATE,
		.parents = { JZ4730_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_MSCR, 8 },
	},

	[JZ4730_CLK_TCU] = {
		"tcu", CGU_CLK_GATE,
		.parents = { JZ4730_CLK_EXT, -1, -1, -1 },
		.gate = { CGU_REG_MSCR, 3 },
	},

	[JZ4730_CLK_AIC] = {
		"aic", CGU_CLK_GATE,
		.parents = { JZ4730_CLK_EXT, -1, -1, -1 },
		.gate = { CGU_REG_MSCR, 18 },
	},
};

static void __init jz4730_cgu_init(struct device_node *np)
{
	int retval;

	cgu = ingenic_cgu_new(jz4730_cgu_clocks,
			      ARRAY_SIZE(jz4730_cgu_clocks), np);
	if (!cgu) {
		pr_err("%s: failed to initialise CGU\n", __func__);
		return;
	}

	retval = ingenic_cgu_register_clocks(cgu);
	if (retval)
		pr_err("%s: failed to register CGU Clocks\n", __func__);

	ingenic_cgu_register_syscore_ops(cgu);
}

CLK_OF_DECLARE_DRIVER(jz4730_cgu, "ingenic,jz4730-cgu", jz4730_cgu_init);
