#include <linux/sysfs.h>
#include "sgxfreq.h"

static int activeidle_start(struct sgxfreq_sgx_data *data);
static void activeidle_stop(void);
static void activeidle_sgx_active(void);
static void activeidle_sgx_idle(void);

static struct activeidle_data {
	unsigned long freq_active;
	unsigned long freq_idle;
	struct mutex mutex;
	bool sgx_active;
} aid;

static struct sgxfreq_governor activeidle_gov = {
	.name =	"activeidle",
	.gov_start = activeidle_start,
	.gov_stop = activeidle_stop,
	.sgx_active = activeidle_sgx_active,
	.sgx_idle = activeidle_sgx_idle,
};

/*********************** begin sysfs interface ***********************/

extern struct kobject *sgxfreq_kobj;

static ssize_t show_freq_active(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%lu\n", aid.freq_active);
}

static ssize_t store_freq_active(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int ret;
	unsigned long freq;

	ret = sscanf(buf, "%lu", &freq);
	if (ret != 1)
		return -EINVAL;

	freq = sgxfreq_get_freq_ceil(freq);

	mutex_lock(&aid.mutex);

	aid.freq_active = freq;
	if (aid.sgx_active)
		sgxfreq_set_freq_request(aid.freq_active);

	mutex_unlock(&aid.mutex);

	return count;
}

static ssize_t show_freq_idle(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	return sprintf(buf, "%lu\n", aid.freq_idle);
}

static ssize_t store_freq_idle(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int ret;
	unsigned long freq;

	ret = sscanf(buf, "%lu", &freq);
	if (ret != 1)
		return -EINVAL;

	freq = sgxfreq_get_freq_floor(freq);

	mutex_lock(&aid.mutex);

	aid.freq_idle = freq;
	if (!aid.sgx_active)
		sgxfreq_set_freq_request(aid.freq_idle);

	mutex_unlock(&aid.mutex);

	return count;
}
static DEVICE_ATTR(freq_active, 0644, show_freq_active, store_freq_active);
static DEVICE_ATTR(freq_idle, 0644, show_freq_idle, store_freq_idle);

static struct attribute *activeidle_attributes[] = {
	&dev_attr_freq_active.attr,
	&dev_attr_freq_idle.attr,
	NULL
};

static struct attribute_group activeidle_attr_group = {
	.attrs = activeidle_attributes,
	.name = "activeidle",
};

/************************ end sysfs interface ************************/

int activeidle_init(void)
{
	int ret;

	mutex_init(&aid.mutex);

	ret = sgxfreq_register_governor(&activeidle_gov);
	if (ret)
		return ret;

	aid.freq_idle = sgxfreq_get_freq_min();
	aid.freq_active = sgxfreq_get_freq_max();

	return 0;
}

int activeidle_deinit(void)
{
	return 0;
}

static int activeidle_start(struct sgxfreq_sgx_data *data)
{
	int ret;

	aid.sgx_active = data->active;

	ret = sysfs_create_group(sgxfreq_kobj, &activeidle_attr_group);
	if (ret)
		return ret;

	if (aid.sgx_active)
		sgxfreq_set_freq_request(aid.freq_active);
	else
		sgxfreq_set_freq_request(aid.freq_idle);

	return 0;
}

static void activeidle_stop(void)
{
	sysfs_remove_group(sgxfreq_kobj, &activeidle_attr_group);
}

static void activeidle_sgx_active(void)
{
	mutex_lock(&aid.mutex);

	aid.sgx_active = true;
	sgxfreq_set_freq_request(aid.freq_active);

	mutex_unlock(&aid.mutex);
}

static void activeidle_sgx_idle(void)
{
	mutex_lock(&aid.mutex);

	aid.sgx_active = false;
	sgxfreq_set_freq_request(aid.freq_idle);

	mutex_unlock(&aid.mutex);
}
