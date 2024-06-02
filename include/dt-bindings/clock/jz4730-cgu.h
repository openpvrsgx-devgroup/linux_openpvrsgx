// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This header provides clock numbers for the ingenic,jz4730-cgu DT binding.
 *
 * They are roughly ordered as:
 *   - external clocks
 *   - PLLs
 *   - muxes/dividers
 *   - gates in order of their bit in the MSCR* registers
 */

#ifndef __DT_BINDINGS_CLOCK_JZ4730_CGU_H__
#define __DT_BINDINGS_CLOCK_JZ4730_CGU_H__

#define JZ4730_CLK_EXT		0
#define JZ4730_CLK_RTC		1
#define JZ4730_CLK_MSC16M	2
#define JZ4730_CLK_MSC24M	3
#define JZ4730_CLK_USB48M	4
#define JZ4730_CLK_PLL		5
#define JZ4730_CLK_PLL_HALF	6
#define JZ4730_CLK_CCLK_PLL	7
#define JZ4730_CLK_CCLK		8
#define JZ4730_CLK_HCLK_PLL	9
#define JZ4730_CLK_HCLK		10
#define JZ4730_CLK_PCLK_PLL	11
#define JZ4730_CLK_PCLK		12
#define JZ4730_CLK_MCLK_PLL	13
#define JZ4730_CLK_MCLK		14
#define JZ4730_CLK_LCD_PLL	15
#define JZ4730_CLK_LCD		16
#define JZ4730_CLK_LCD_PCLK	17
#define JZ4730_CLK_I2S		18
#define JZ4730_CLK_SPI		19
#define JZ4730_CLK_DMA		20
#define JZ4730_CLK_MMC		21
#define JZ4730_CLK_UHC_IN	22
#define JZ4730_CLK_UHC		23
#define JZ4730_CLK_UART0	24
#define JZ4730_CLK_UART1	25
#define JZ4730_CLK_UART2	26
#define JZ4730_CLK_UART3	27
#define JZ4730_CLK_I2C		28
#define JZ4730_CLK_TCU		29
#define JZ4730_CLK_EXT_128	30
#define JZ4730_CLK_WDT		31
#define JZ4730_CLK_AIC		32

#endif /* __DT_BINDINGS_CLOCK_JZ4730_CGU_H__ */
