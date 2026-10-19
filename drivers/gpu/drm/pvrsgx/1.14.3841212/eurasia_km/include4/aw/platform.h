/*************************************************************************/ /*!
@File           platform.h
@Title          Platform Header
@Copyright      Copyright (c) Allwinnertech Co., Ltd. All Rights Reserved
@Description    Define structs of GPU for Allwinnertech platform
@License        GPLv2

The contents of this file is used under the terms of the GNU General Public 
License Version 2 ("GPL") in which case the provisions of GPL are applicable
instead of those above.
*/ /**************************************************************************/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/clk/sunxi_name.h>
#include <linux/delay.h>
#include <linux/stat.h>
#include <linux/pm_runtime.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sys_config.h>
#include <mach/sunxi-smc.h>

#ifdef CONFIG_CPU_BUDGET_THERMAL
#include <linux/cpu_budget_cooling.h>
#endif /* CONFIG_CPU_BUDGET_THERMAL */

#include "services_headers.h"
#include "sysinfo.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

struct aw_clk_data
{
	const char *clk_name;    /* Clock name */

	const char *clk_id;      /* Clock ID, which is in the system configuration head file */

	const char *clk_parent_id;

	struct clk *clk_handle;  /* Clock handle, we can set the frequency value via it */

	/* If the value is 1, the reset API while handle it to reset corresponding module */   
	bool       need_reset;

	/* The meaning of the values are as follows:
	*  0: don't need to set frequency.
	*  1: use the public frequency
	*  >1: use the private frequency, the value is for frequency.
	*/
	u32       expected_freq;
};

struct aw_vf
{
	u32 vol;   /* mV */
	u32 freq;  /* MHz */
};

struct aw_vf_data
{
	struct aw_vf normal;
	struct aw_vf extreme;
};

struct reg_data
{
	u8   bit;
	void *addr;
};

#ifdef CONFIG_CPU_BUDGET_THERMAL
/* The struct is for temperature-frequency table */
struct aw_tf_table
{
    u8  temp;
    u32 freq;
};
#endif /* CONFIG_CPU_BUDGET_THERMAL */

struct aw_tempctrl_data
{
    bool temp_ctrl_status;
    u8   count;     /* The data in tf_table to use */
};

struct aw_private_data
{
	bool   scene_ctrl_status;
	u32    max_freq; /* MHz */
	u8     sensor_num;
	struct regulator *regulator;
	char   *regulator_id;
	struct mutex dvfs_lock;
	struct reg_data poweroff_gate;
	struct aw_tempctrl_data tempctrl_data;
};

#if defined(CONFIG_ARCH_SUN8IW6P1)
#include "sun8i/sun8iw6p1.h"
#else
#error "please select a platform\n"
#endif

#endif