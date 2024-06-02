// SPDX-License-Identifier: GPL-2.0
/*
 * JZ4780 CPU idle driver
 * Copyright (C) 2015 Imagination Technologies
 * Author: Alex Smith <alex.smith@imgtec.com>
 * Copyright (c) 2020 周琰杰 (Zhou Yanjie) <zhouyanjie@wanyeetech.com>
 */

#include <linux/cpuidle.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sched/idle.h>

#include <asm/idle.h>
#include <asm/mipsregs.h>

/*
 * The JZ4780 has a high overhead to entering just the basic MIPS wait on SMP,
 * due to the requirement to flush out dirty lines from the dcache before
 * waiting. Therefore, we try to mitigate this overhead by using a simple
 * polling loop for short waits.
 */
static int jz4780_cpuidle_poll_enter(struct cpuidle_device *dev,
				     struct cpuidle_driver *drv, int index)
{
	if (!current_set_polling_and_test())
		while (!need_resched() && !(read_c0_cause() & read_c0_status() & CAUSEF_IP))
			cpu_relax();

	current_clr_polling();
	local_irq_enable();

	return index;
}

static struct cpuidle_driver jz4780_cpuidle_driver = {
	.name = "jz4780_cpuidle",
	.owner = THIS_MODULE,
	.states = {
		{
			.enter = jz4780_cpuidle_poll_enter,
			.exit_latency = 1,
			.target_residency = 1,
			.power_usage = UINT_MAX,
			.name = "poll",
			.desc = "polling loop",
		},
		{
			.enter = mips_cpuidle_wait_enter,
			.exit_latency = 50,
			.target_residency = 300,
			.power_usage = UINT_MAX,
			.name = "wait",
			.desc = "MIPS wait",
		},
	},
	.state_count = 2,
};

static int __init jz4780_cpuidle_init(void)
{
	int ret;

	ret = cpuidle_register(&jz4780_cpuidle_driver, NULL);
	if (ret) {
		pr_err("Failed to register JZ4780 idle driver: %d\n", ret);
		return ret;
	}

	pr_info("JZ4780 idle driver registered\n");

	return 0;
}
device_initcall(jz4780_cpuidle_init);
