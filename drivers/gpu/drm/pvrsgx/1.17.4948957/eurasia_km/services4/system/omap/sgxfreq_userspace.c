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

#include <linux/sysfs.h>
#include "sgxfreq.h"


static int userspace_start(struct sgxfreq_sgx_data *data);
static void userspace_stop(void);


static struct sgxfreq_governor userspace_gov = {
	.name =	"userspace",
	.gov_start = userspace_start,
	.gov_stop = userspace_stop,
};


static struct userspace_data {
	unsigned long freq_user; /* in Hz */
} usd;


/*********************** begin sysfs interface ***********************/

extern struct kobject *sgxfreq_kobj;


static ssize_t show_frequency_set(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "%lu\n", usd.freq_user);
}


static ssize_t store_frequency_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long freq;

	ret = sscanf(buf, "%lu", &freq);
	if (ret != 1)
		return -EINVAL;

	if (freq > sgxfreq_get_freq_max())
		freq = sgxfreq_get_freq_max();
	usd.freq_user = sgxfreq_set_freq_request(freq);
	trace_printk("USERSPACE: new freq=%luHz.\n", usd.freq_user);

	return count;
}


static DEVICE_ATTR(frequency_set, 0644,
	show_frequency_set, store_frequency_set);


static struct attribute *userspace_attributes[] = {
	&dev_attr_frequency_set.attr,
	NULL
};


static struct attribute_group userspace_attr_group = {
	.attrs = userspace_attributes,
	.name = "userspace",
};

/************************ end sysfs interface ************************/


int userspace_init(void)
{
	int ret;

	ret = sgxfreq_register_governor(&userspace_gov);
	if (ret)
		return ret;
	return 0;
}


int userspace_deinit(void)
{
	return 0;
}


static int userspace_start(struct sgxfreq_sgx_data *data)
{
	int ret;

	usd.freq_user = sgxfreq_get_freq();

	ret = sysfs_create_group(sgxfreq_kobj, &userspace_attr_group);
	if (ret)
		return ret;

	trace_printk("USERSPACE: started.\n");

	return 0;
}


static void userspace_stop(void)
{
	sysfs_remove_group(sgxfreq_kobj, &userspace_attr_group);

	trace_printk("USERSPACE: stopped.\n");
}
