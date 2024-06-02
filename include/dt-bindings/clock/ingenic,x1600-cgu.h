/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides clock numbers for the ingenic,x1600-cgu DT binding.
 *
 * They are roughly ordered as:
 *   - external clocks
 *   - PLLs
 *   - muxes/dividers in the order they appear in the x1600 programmers manual
 *   - gates in order of their bit in the CLKGR* registers
 */

#ifndef __DT_BINDINGS_CLOCK_X1600_CGU_H__
#define __DT_BINDINGS_CLOCK_X1600_CGU_H__

/*
 * unwritten rules
 * must start with 0
 * must not have "holes"
 * all clock parents must have lower number than a clock itself so that parents are initialized before
 */

#define X1600_CLK_EXCLK			0
#define X1600_CLK_RTCLK			1
#define X1600_CLK_12M			2
#define X1600_CLK_APLL			3
#define X1600_CLK_EPLL			4
#define X1600_CLK_MPLL			5
#define X1600_CLK_SCLKA			6
#define X1600_CLK_CPUMUX		7
#define X1600_CLK_CPU			8
#define X1600_CLK_L2CACHE		9
#define X1600_CLK_AHB0			10
#define X1600_CLK_AHB2PMUX		11
#define X1600_CLK_AHB2			12
#define X1600_CLK_PCLK			13
#define X1600_CLK_DDR			14
#define X1600_CLK_I2S0			15
#define X1600_CLK_I2S1			16
#define X1600_CLK_LCDPIXCLK		17
#define X1600_CLK_MAC			18
#define X1600_CLK_MSC0			19
#define X1600_CLK_MSC1			20
#define X1600_CLK_SSI0			21
#define X1600_CLK_CIMMCLK		22
#define X1600_CLK_EXCLK_DIV512		23
#define X1600_CLK_RTC			24
#define X1600_CLK_OTGPHY		25
#define X1600_CLK_NEMC			26
#define X1600_CLK_OTG0			27
#define X1600_CLK_SMB0			28
#define X1600_CLK_SMB1			29
#define X1600_CLK_AIC			30
#define X1600_CLK_SADC			31
#define X1600_CLK_UART0			32
#define X1600_CLK_UART1			33
#define X1600_CLK_UART2			34
#define X1600_CLK_UART3			35
#define X1600_CLK_PDMA			36
#define X1600_CLK_AES			37

/*
#define X1600_CLK_LCD			15
#define X1600_CLK_MSCMUX		16
#define X1600_CLK_OTG			19
#define X1600_CLK_SSIPLL		20
#define X1600_CLK_SSIPLL_DIV2	21
#define X1600_CLK_SSIMUX		22
#define X1600_CLK_EMC			23
#define X1600_CLK_EFUSE			24
#define X1600_CLK_SFC			25
#define X1600_CLK_I2C0			26
#define X1600_CLK_I2C1			27
#define X1600_CLK_I2C2			28
#define X1600_CLK_TCU			32
#define X1600_CLK_SSI			33
#define X1600_CLK_OST			34
#define X1600_CLK_I2SPLLMUX		39
#define X1600_CLK_I2SPLL		40
#define X1600_CLK_I2S			41
*/

#endif /* __DT_BINDINGS_CLOCK_X1600_CGU_H__ */
