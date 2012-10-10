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
	.name = "gpu_cooling",
	.domain_name = "gpu",
	.dev_ops = &cool_dev_ops,
};

int cool_init(void)
{
	cd.freq_cnt = sgxfreq_get_freq_list(&cd.freq_list);
	if (!cd.freq_cnt || !cd.freq_list)
		return -EINVAL;

	return thermal_cooling_dev_register(&cool_dev);
}

void cool_deinit(void)
{
	thermal_cooling_dev_unregister(&cool_dev);
}

static int cool_device(struct thermal_dev *dev, int cooling_level)
{
	int freq_max_index, freq_limit_index;

	freq_max_index = cd.freq_cnt - 1;
	freq_limit_index = freq_max_index - cooling_level;
	if (freq_limit_index < 0)
		freq_limit_index = 0;

	sgxfreq_set_freq_limit(cd.freq_list[freq_limit_index]);

	return 0;
}
