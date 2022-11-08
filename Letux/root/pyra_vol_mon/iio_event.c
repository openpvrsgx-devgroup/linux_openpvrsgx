#include "iio_event.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/iio/events.h>
#include <sys/ioctl.h>

#include "iio_utils.h"

#define PYRA_DEVICE_NAME	"palmas-gpadc"

struct pyra_iio_event_handle {
	const char *dev_dir_name;
	const char *input;
	const char *upper_enable;
	const char *upper_threshold;
	const char *lower_enable;
	const char *lower_threshold;
};

__attribute__((format(printf, 2, 3)))
static int safe_asprintf(char **str, const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vasprintf(str, fmt, ap);
	if (n < 0)
		*str = NULL;
	va_end(ap);
	return n;
}

static int open_event_fd(int dev_num)
{
	int ret;
	int fd, event_fd;
	char *chrdev_name;

	ret = asprintf(&chrdev_name, "/dev/iio:device%d", dev_num);
	if (ret < 0)
		return -ENOMEM;

	fd = open(chrdev_name, 0);
	if (fd == -1) {
		ret = -errno;
		fprintf(stderr, "Failed to open %s\n", chrdev_name);
		goto out_free;
	}

	ret = ioctl(fd, IIO_GET_EVENT_FD_IOCTL, &event_fd);
	if (ret == -1 || event_fd == -1) {
		ret = -errno;
		if (ret == -ENODEV)
			fprintf(stderr,
				"This device does not support events\n");
		else
			fprintf(stderr, "Failed to retrieve event fd\n");
		if (close(fd) == -1)
			perror("Failed to close character device file");

		goto out_free;
	}

	if (close(fd) == -1)  {
		ret = -errno;
		if (close(event_fd) == -1)
			perror("Failed to close event file descriptor");
		goto out_free;
	}

	ret = event_fd;

out_free:
	free(chrdev_name);
	return ret;
}

static int enable_threshold(const char* dev_dir_name,
			    const char* enable,
			    const char* threshold,
			    const int value)
{
	int ret;

	/* it's currently necessary to disable threshold before updating it. Future
	* updates to the palmas_gpadc module might fix this. */
	ret = write_sysfs_int_and_verify(enable, dev_dir_name, 0);
	if (ret < 0)
		return ret;

	ret = write_sysfs_int_and_verify(threshold, dev_dir_name, value);
	if (ret < 0)
		return ret;

	ret = write_sysfs_int_and_verify(enable, dev_dir_name, 1);
	if (ret < 0)
		return ret;

	return 0;
}

static int disable_threshold(const char* dev_dir_name,
			     const char* enable)
{
	int ret;

	ret = write_sysfs_int_and_verify(enable, dev_dir_name, 0);
	if (ret < 0)
		return ret;

	return 0;
}

int pyra_iio_event_open(struct pyra_iio_event_handle** handle, int channel)
{
	struct pyra_iio_event_handle* h;
	const char* device_name = PYRA_DEVICE_NAME;
	char* dev_dir_name = NULL;
	char* input_file = NULL;
	char* lower_threshold_value = NULL;
	char* lower_threshold_enable = NULL;
	char* upper_threshold_value = NULL;
	char* upper_threshold_enable = NULL;
	int dev_num;
	int ret;

	dev_num = find_type_by_name(device_name, "iio:device");
	if (dev_num < 0) {
		fprintf(stderr, "Could not find IIO device with name %s\n", device_name);
		return -ENODEV;
	}

	printf("Found IIO device with name %s with device number %d\n",
		device_name, dev_num);

	ret = asprintf(&dev_dir_name, "%siio:device%d", iio_dir, dev_num);
	if (ret < 0)
		return ret;

	ret = safe_asprintf(&input_file, "in_voltage%d_input", channel);
	if (ret < 0)
		goto error_free;

	ret = safe_asprintf(&lower_threshold_enable,
			"events/in_voltage%d_thresh_falling_en", channel);
	if (ret < 0)
		goto error_free;

	ret = safe_asprintf(&lower_threshold_value,
			"events/in_voltage%d_thresh_falling_value", channel);
	if (ret < 0)
		goto error_free;

	ret = safe_asprintf(&upper_threshold_enable,
			"events/in_voltage%d_thresh_rising_en", channel);
	if (ret < 0)
		goto error_free;

	ret = safe_asprintf(&upper_threshold_value,
			"events/in_voltage%d_thresh_rising_value", channel);
	if (ret < 0)
		goto error_free;

	ret = open_event_fd(dev_num);
	if (ret < 0)
		goto error_free;


	h = (struct pyra_iio_event_handle*)malloc(sizeof(*h));
	if (!h) {
		ret = -ENOMEM;
		goto error_free;
	}

	h->dev_dir_name = dev_dir_name;
	h->input = input_file;
	h->upper_enable = upper_threshold_enable;
	h->upper_threshold = upper_threshold_value;
	h->lower_enable = lower_threshold_enable;
	h->lower_threshold = lower_threshold_value;

	*handle = h;

	return ret;
error_free:
	free(upper_threshold_value);
	free(upper_threshold_enable);
	free(lower_threshold_value);
	free(lower_threshold_enable);
	free(input_file);
	free(dev_dir_name);
	return ret;
}

void pyra_iio_event_free(struct pyra_iio_event_handle* handle)
{
	free((char*)handle->lower_threshold);
	free((char*)handle->lower_enable);
	free((char*)handle->upper_threshold);
	free((char*)handle->upper_enable);
	free((char*)handle->input);
	free((char*)handle->dev_dir_name);
	free(handle);
}

int pyra_iio_get_value(const struct pyra_iio_event_handle* handle)
{
	const struct pyra_iio_event_handle* iio = handle;
	return read_sysfs_posint(iio->input, iio->dev_dir_name);
}

int pyra_iio_enable_upper_threshold(const struct pyra_iio_event_handle* handle, int threshold)
{
	const struct pyra_iio_event_handle* iio = handle;

	return enable_threshold(iio->dev_dir_name,
				iio->upper_enable,
				iio->upper_threshold,
				threshold);
}

int pyra_iio_disable_upper_threshold(const struct pyra_iio_event_handle* handle)
{
	const struct pyra_iio_event_handle* iio = handle;

	return disable_threshold(iio->dev_dir_name, iio->upper_enable);
}

int pyra_iio_enable_lower_threshold(const struct pyra_iio_event_handle* handle, int threshold)
{
	const struct pyra_iio_event_handle* iio = handle;

	return enable_threshold(iio->dev_dir_name,
				iio->lower_enable,
				iio->lower_threshold,
				threshold);
}

int pyra_iio_disable_lower_threshold(const struct pyra_iio_event_handle* handle)
{
	const struct pyra_iio_event_handle* iio = handle;

	return disable_threshold(iio->dev_dir_name, iio->lower_enable);
}
