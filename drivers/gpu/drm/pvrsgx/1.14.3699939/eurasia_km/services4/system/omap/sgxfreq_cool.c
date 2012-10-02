/*
 * Copyright (C) 2012 Texas Instruments, Inc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0))

#include <linux/thermal.h>

static struct cool_data {
	int freq_cnt;
	unsigned long *freq_list;
	unsigned long state;
	struct thermal_cooling_device *cdev;
} cd;

static int sgxfreq_get_max_state(struct thermal_cooling_device *cdev,
		unsigned long *state)
{
	*state = cd.freq_cnt - 1;
	return 0;
}

static int sgxfreq_get_cur_state(struct thermal_cooling_device *cdev,
		unsigned long *state)
{
	*state = cd.state;
	return 0;
}

static int sgxfreq_set_cur_state(struct thermal_cooling_device *cdev,
		unsigned long state)
{
	int freq_max_index, freq_limit_index;

	freq_max_index = cd.freq_cnt - 1;

	freq_limit_index = freq_max_index - (unsigned int)state;

	if (freq_limit_index < 0)
		freq_limit_index = 0;

	sgxfreq_set_freq_limit(cd.freq_list[freq_limit_index]);

	cd.state = state;
	return 0;
}


static const struct thermal_cooling_device_ops sgxfreq_cooling_ops = {
	.get_max_state = sgxfreq_get_max_state,
	.get_cur_state = sgxfreq_get_cur_state,
	.set_cur_state = sgxfreq_set_cur_state,
};

int cool_init(void)
{
	int ret;
	struct thermal_zone_device *tz;

	cd.freq_cnt = sgxfreq_get_freq_list(&cd.freq_list);
	if (!cd.freq_cnt || !cd.freq_list)
		return -EINVAL;

	cd.cdev = thermal_cooling_device_register("gpu", (void *)NULL, &sgxfreq_cooling_ops);

	if(IS_ERR(cd.cdev)) {
		pr_err("sgxfreq: Error while regeistering cooling device: %ld\n", PTR_ERR(cd.cdev));
		return -1;
	}

	tz = thermal_zone_get_zone_by_name("gpu");
	if(IS_ERR(tz)) {
		pr_err("sgxfreq: Error while trying to obtain zone device: %ld\n", PTR_ERR(tz));
		return -1;
	}

	ret = thermal_zone_bind_cooling_device(tz, 0, cd.cdev, THERMAL_NO_LIMIT, THERMAL_NO_LIMIT);
	if (ret)
	{
		pr_err("sgxfreq: Error binding cooling device: %d\n", ret);
	}

	return 0;
}

void cool_deinit(void)
{
	thermal_cooling_device_unregister(cd.cdev);
}
#else //if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0))
#include <linux/thermal_framework.h>

static int cool_device(struct thermal_dev *dev, int cooling_level);

static struct cool_data {
	int freq_cnt;
	unsigned long *freq_list;
} cd;

static struct thermal_dev_ops cool_dev_ops = {
	.cool_device = cool_device,
};

static struct thermal_dev cool_dev = {
	.name = "gpu_cooling.0",
	.domain_name = "gpu",
	.dev_ops = &cool_dev_ops,
};

static struct thermal_dev case_cool_dev = {
	.name = "gpu_cooling.1",
	.domain_name = "case",
	.dev_ops = &cool_dev_ops,
};

static unsigned int gpu_cooling_level;
#if defined(CONFIG_CASE_TEMP_GOVERNOR)
static unsigned int case_cooling_level;
#endif

int cool_init(void)
{
	int ret;
	cd.freq_cnt = sgxfreq_get_freq_list(&cd.freq_list);
	if (!cd.freq_cnt || !cd.freq_list)
		return -EINVAL;

	ret = thermal_cooling_dev_register(&cool_dev);
	if (ret)
		return ret;

	return thermal_cooling_dev_register(&case_cool_dev);
}

void cool_deinit(void)
{
	thermal_cooling_dev_unregister(&cool_dev);
	thermal_cooling_dev_unregister(&case_cool_dev);
}

static int cool_device(struct thermal_dev *dev, int cooling_level)
{
	int freq_max_index, freq_limit_index;

#if defined(CONFIG_CASE_TEMP_GOVERNOR)
	if (!strcmp(dev->domain_name, "case"))
	{
		int tmp = 0;
		tmp = cooling_level - case_subzone_number;
		if (tmp < 0)
			tmp = 0;
		case_cooling_level = tmp;
	}
	else
#endif
	{
               gpu_cooling_level = cooling_level;
	}

	freq_max_index = cd.freq_cnt - 1;
#if defined(CONFIG_CASE_TEMP_GOVERNOR)
	if (case_cooling_level > gpu_cooling_level)
	{
		freq_limit_index = freq_max_index - case_cooling_level;
	}
	else
#endif
	{
		freq_limit_index = freq_max_index - gpu_cooling_level;
	}

	if (freq_limit_index < 0)
		freq_limit_index = 0;

	sgxfreq_set_freq_limit(cd.freq_list[freq_limit_index]);

	return 0;
}
#endif //if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0))
