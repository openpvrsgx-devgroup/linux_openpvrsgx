/*************************************************************************/ /*!
@File           sun8iw6p1.h
@Title          SUN8IW6P1 Header
@Copyright      Copyright (c) Allwinnertech Co., Ltd. All Rights Reserved
@Description    Define Detailed data about GPU for Allwinnertech platforms
@License        GPLv2

The contents of this file is used under the terms of the GNU General Public 
License Version 2 ("GPL") in which case the provisions of GPL are applicable
instead of those above.
*/ /**************************************************************************/

#ifndef _SUN8IW6P1_H_
#define _SUN8IW6P1_H_

static struct aw_vf_data vf_data =
{
	.normal =
	{
		.vol  = 900,
		.freq = 624,
	},
	.extreme =
	{
		.vol  = 1100,
		.freq = 744,
	},
};

static struct aw_private_data private_data =
{
	.scene_ctrl_status = 1,
	.sensor_num        = 2,
	.regulator         = NULL,
	.regulator_id      = "vdd-gpu",
	.poweroff_gate     =
	{
		.bit  = 0,
		.addr = SUNXI_R_PRCM_VBASE + 0x118,
	},
	.tempctrl_data     =
    {
        .temp_ctrl_status = 1,
    },
};

static struct aw_clk_data clk_data[] =
{
	{
		.clk_name      = "pll",
		.clk_id        = PLL_GPU_CLK,
		.clk_parent_id = NULL,
		.clk_handle    = NULL,
		.need_reset    = 0,
		.expected_freq = 1,
	},
	{
		.clk_name      = "core",
		.clk_id        = GPUCORE_CLK,
		.clk_parent_id = PLL_GPU_CLK,
		.clk_handle    = NULL,
		.need_reset    = 0,
		.expected_freq = 1,
	},
	{
		.clk_name      = "mem",
		.clk_id        = GPUMEM_CLK,
		.clk_parent_id = PLL_GPU_CLK,
		.clk_handle    = NULL,
		.need_reset    = 0,
		.expected_freq = 1,
	},
	{
		.clk_name      = "hyd",
		.clk_id        = GPUHYD_CLK,
		.clk_parent_id = PLL_GPU_CLK,
		.clk_handle    = NULL,
		.need_reset    = 1,
		.expected_freq = 1,
	},
};

#ifdef CONFIG_CPU_BUDGET_THERMAL
static struct aw_tf_table tf_table[] =
{
    {
        .temp = 85,
        .freq = 504,
    },
    {
        .temp = 95,
        .freq = 288,
    },
};
#endif /* CONFIG_CPU_BUDGET_THERMAL */

#endif