#include <linux/sysfs.h>
#include "sgxfreq.h"

static int onoff_start(bool sgx_clk_on);
static void onoff_stop(void);
static void onoff_sgx_clk_on(void);
static void onoff_sgx_clk_off(void);

static struct onoff_data {
	unsigned long freq_off;
	unsigned long freq_on;
	struct mutex mutex;
	bool sgx_clk_on;
} ood;

static struct sgxfreq_governor onoff_gov = {
	.name =	"onoff",
	.gov_start = onoff_start,
	.gov_stop = onoff_stop,
	.sgx_clk_on = onoff_sgx_clk_on,
	.sgx_clk_off = onoff_sgx_clk_off,
};

/*********************** begin sysfs interface ***********************/

extern struct kobject *sgxfreq_kobj;

static ssize_t show_freq_on(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "%lu\n", ood.freq_on);
}

static ssize_t store_freq_on(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int ret;
	unsigned long freq;

	ret = sscanf(buf, "%lu", &freq);
	if (ret != 1)
		return -EINVAL;

	freq = sgxfreq_get_freq_ceil(freq);

	mutex_lock(&ood.mutex);

	ood.freq_on = freq;
	if (ood.sgx_clk_on)
		sgxfreq_set_freq_request(ood.freq_on);

	mutex_unlock(&ood.mutex);

	return count;
}

static ssize_t show_freq_off(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	return sprintf(buf, "%lu\n", ood.freq_off);
}

static ssize_t store_freq_off(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	unsigned long freq;

	ret = sscanf(buf, "%lu", &freq);
	if (ret != 1)
		return -EINVAL;

	freq = sgxfreq_get_freq_floor(freq);

	mutex_lock(&ood.mutex);

	ood.freq_off = freq;
	if (!ood.sgx_clk_on)
		sgxfreq_set_freq_request(ood.freq_off);

	mutex_unlock(&ood.mutex);

	return count;
}
static DEVICE_ATTR(freq_on, 0644, show_freq_on, store_freq_on);
static DEVICE_ATTR(freq_off, 0644, show_freq_off, store_freq_off);

static struct attribute *onoff_attributes[] = {
	&dev_attr_freq_on.attr,
	&dev_attr_freq_off.attr,
	NULL
};

static struct attribute_group onoff_attr_group = {
	.attrs = onoff_attributes,
	.name = "onoff",
};

/************************ end sysfs interface ************************/

int onoff_init(void)
{
	int ret;

	mutex_init(&ood.mutex);

	ret = sgxfreq_register_governor(&onoff_gov);
	if (ret)
		return ret;

	ood.freq_off = sgxfreq_get_freq_min();
	ood.freq_on = sgxfreq_get_freq_max();

	return 0;
}

int onoff_deinit(void)
{
	return 0;
}

static int onoff_start(bool sgx_clk_on)
{
	int ret;

	ood.sgx_clk_on = sgx_clk_on;

	ret = sysfs_create_group(sgxfreq_kobj, &onoff_attr_group);
	if (ret)
		return ret;

	if (sgx_clk_on)
		sgxfreq_set_freq_request(ood.freq_on);
	else
		sgxfreq_set_freq_request(ood.freq_off);

	return 0;
}

static void onoff_stop(void)
{
	sysfs_remove_group(sgxfreq_kobj, &onoff_attr_group);
}

static void onoff_sgx_clk_on(void)
{
	mutex_lock(&ood.mutex);

	ood.sgx_clk_on = true;
	sgxfreq_set_freq_request(ood.freq_on);

	mutex_unlock(&ood.mutex);
}

static void onoff_sgx_clk_off(void)
{
	mutex_lock(&ood.mutex);

	ood.sgx_clk_on = false;
	sgxfreq_set_freq_request(ood.freq_off);

	mutex_unlock(&ood.mutex);
}
