#include <linux/sysfs.h>
#include "sgxfreq.h"


static int userspace_start(struct sgxfreq_sgx_data *data);
static void userspace_stop(void);
static void userspace_sgx_clk_on(void);
static void userspace_sgx_clk_off(void);
static void userspace_sgx_active(void);
static void userspace_sgx_idle(void);


static struct sgxfreq_governor userspace_gov = {
	.name =	"userspace",
	.gov_start = userspace_start,
	.gov_stop = userspace_stop,
	.sgx_clk_on = userspace_sgx_clk_on,
	.sgx_clk_off = userspace_sgx_clk_off,
	.sgx_active = userspace_sgx_active,
	.sgx_idle = userspace_sgx_idle,
};


static struct userspace_data {
	unsigned long freq_user; /* in KHz */
	struct mutex mutex;
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

	mutex_lock(&odd.mutex);

	if (freq > sgxfreq_get_freq_max())
		freq = sgxfreq_get_freq_max();
	usd.freq_user = sgxfreq_set_freq_request(freq);
	trace_printk("USERSPACE: new freq=%luHz.\n", usd.freq_user);

	mutex_unlock(&odd.mutex);

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

	mutex_init(&odd.mutex);

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
	usd.freq_user = sgxfreq_set_freq_request(sgxfreq_get_freq_min());
	sysfs_remove_group(sgxfreq_kobj, &userspace_attr_group);

	trace_printk("USERSPACE: stopped.\n");
}


static void userspace_sgx_clk_on(void)
{
	mutex_lock(&ood.mutex);

	sgxfreq_set_freq_request(usd.freq_user);

	mutex_unlock(&ood.mutex);
}


static void userspace_sgx_clk_off(void)
{
	mutex_lock(&ood.mutex);

	sgxfreq_set_freq_request(sgxfreq_get_freq_min());

	mutex_unlock(&ood.mutex);
}


static void userspace_sgx_active(void)
{
	mutex_lock(&aid.mutex);

	sgxfreq_set_freq_request(usd.freq_user);

	mutex_unlock(&aid.mutex);
}


static void userspace_sgx_idle(void)
{
	mutex_lock(&aid.mutex);

	sgxfreq_set_freq_request(sgxfreq_get_freq_min());

	mutex_unlock(&aid.mutex);
}
