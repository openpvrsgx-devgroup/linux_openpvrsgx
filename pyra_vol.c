#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "iio_utils.h"
#include <linux/iio/events.h>
#include <linux/iio/types.h>

#define STEP (25)

static bool event_is_ours(struct iio_event_data *event, int channel)
{
	int chan = IIO_EVENT_CODE_EXTRACT_CHAN(event->id);
	enum iio_chan_type type = IIO_EVENT_CODE_EXTRACT_CHAN_TYPE(event->id);
	enum iio_event_type ev_type = IIO_EVENT_CODE_EXTRACT_TYPE(event->id);
	enum iio_event_direction dir = IIO_EVENT_CODE_EXTRACT_DIR(event->id);

	if (chan != channel)
		return false;
 	if (type != IIO_VOLTAGE)
		return false;
	if (ev_type != IIO_EV_TYPE_THRESH)
		return false;
	if (dir != IIO_EV_DIR_FALLING && dir != IIO_EV_DIR_RISING)
		return false;

	return true;
}

static int parse_channel_arg(const char* str)
{
	char *endptr;
	long val;
	int ret;
	
	errno = 0;
	val = strtol(str, &endptr, 10);
	
	if (errno != 0) {
		ret = errno;
		perror("strtol");
		return ret;
	}
	
	if (endptr == str) {
		fprintf(stderr, "Channel '%s' is not a number\n", str);
		return -EINVAL;
	}
	
	if (val < 0 || val > INT_MAX) {
		fprintf(stderr, "Channel %ld is not a valid channel\n", val);
		return -EINVAL;
	}
	
	return val;
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
		goto error_free;
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

		goto error_free;
	}

	if (close(fd) == -1)  {
		ret = -errno;
		if (close(event_fd) == -1)
			perror("Failed to close event file descriptor");
		goto error_free;
	}

	ret = event_fd;

error_free:
	free(chrdev_name);
	return ret;
}

static int read_value_and_update_thresholds(
		const char* dev_dir_name,
		const char* input,
		const char* upper_enable,
		const char* upper_threshold,
		const char* lower_enable,
		const char* lower_threshold)
{
	int ret;
	int value;

	ret = read_sysfs_posint(input, dev_dir_name);
	if (ret < 0) {
		fprintf(stderr, "Error reading current value: %d\n", ret);
		return -EAGAIN;
	}
	
	value = ret;

	/* NOTE:
	 * at the time of this writing there is a bug in the palmas-gpadc
	 * driver which makes writing and reading threshold values return
	 * different things. This means we can't verify what we wrote.
	 * I'm sorry.
	 */

	/* update upper threshold */
	ret = write_sysfs_int_and_verify(upper_enable, dev_dir_name, 0);
	if (ret < 0) 
		fprintf(stderr, "Failed to disable upper threshold: %d\n", ret);

	ret = write_sysfs_int(upper_threshold, dev_dir_name, value + STEP);
	if (ret < 0) 
		fprintf(stderr, "Failed to write upper threshold: %d\n", ret);

	ret = write_sysfs_int_and_verify(upper_enable, dev_dir_name, 1);
	if (ret < 0) 
		fprintf(stderr, "Failed to enable upper threshold: %d\n", ret);

	/* update lower threshold */
	ret = write_sysfs_int_and_verify(lower_enable, dev_dir_name, 0);
	if (ret < 0) 
		fprintf(stderr, "Failed to disable lower threshold: %d\n", ret);

	if (value > STEP) {
		ret = write_sysfs_int(lower_threshold, dev_dir_name, value - STEP);
		if (ret < 0) 
			fprintf(stderr, "Failed to write lower threshold: %d\n", ret);

		ret = write_sysfs_int_and_verify(lower_enable, dev_dir_name, 1);
		if (ret < 0) 
			fprintf(stderr, "Failed to enable lower threshold: %d\n", ret);
	}

	return value;
}

static int execute_callback(const char* prog, int value)
{
	pid_t my_pid;
	char value_string[32];

	snprintf(value_string, sizeof(value_string), "%d", value);

	if (0 == (my_pid = fork())) {
		if (-1 == execle(prog, basename(prog), value_string, 0, NULL)) {
			perror("child process execve failed");
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct iio_event_data event;
	const char *device_name;
	const char *executable;
	char *dev_dir_name = NULL;
	char input_file[256];
	char lower_threshold_value[256];
	char lower_threshold_enable[256];
	char upper_threshold_value[256];
	char upper_threshold_enable[256];
	int ret;
	int dev_num;
	int event_fd;
	int channel;

	if (argc <= 2) {
		fprintf(stderr, "Usage: %s <channel> <executable>\n", argv[0]);
		return -1;
	}

	device_name = "palmas-gpadc";
	executable = argv[2];

	errno = 0;
	channel = parse_channel_arg(argv[1]);
	if (channel < 0)
		return channel;

	dev_num = find_type_by_name(device_name, "iio:device");
	if (dev_num < 0) {
		printf("Could not find IIO device with name %s\n", device_name);
		return -ENODEV;
	}

	printf("Found IIO device with name %s with device number %d\n",
		device_name, dev_num);

	ret = asprintf(&dev_dir_name, "%siio:device%d", iio_dir, dev_num);
	if (ret < 0)
		return -ENOMEM;

	ret = snprintf(input_file, sizeof(input_file),
			"in_voltage%d_input", channel);
	if (ret < 0)
		goto error_free;

	ret = snprintf(lower_threshold_enable, sizeof(lower_threshold_enable),
			"events/in_voltage%d_thresh_falling_en", channel);
	if (ret < 0)
		goto error_free;

	ret = snprintf(lower_threshold_value, sizeof(lower_threshold_value),
			"events/in_voltage%d_thresh_falling_value", channel);
	if (ret < 0)
		goto error_free;

	ret = snprintf(upper_threshold_enable, sizeof(upper_threshold_enable),
			"events/in_voltage%d_thresh_rising_en", channel);
	if (ret < 0)
		goto error_free;

	ret = snprintf(upper_threshold_value, sizeof(upper_threshold_value),
			"events/in_voltage%d_thresh_rising_value", channel);
	if (ret < 0)
		goto error_free;

	event_fd = open_event_fd(dev_num);
	if (event_fd < 0) {
		ret = event_fd;
		goto error_free;
	}

	read_value_and_update_thresholds(dev_dir_name, input_file,
			upper_threshold_enable, upper_threshold_value,
			lower_threshold_enable, lower_threshold_value);

	while (true) {
		int value;

		ret = read(event_fd, &event, sizeof(event));
		if (ret == -1) {
			if (errno == EAGAIN) {
				fprintf(stderr, "nothing available\n");
				continue;
			} else {
				ret = -errno;
				perror("Failed to read event from device");
				break;
			}
		}

		if (ret != sizeof(event)) {
			fprintf(stderr, "Reading event failed!\n");
			ret = -EIO;
			break;
		}

		if (!event_is_ours(&event, channel))
			continue;

		value = read_value_and_update_thresholds(dev_dir_name, input_file,
				upper_threshold_enable, upper_threshold_value,
				lower_threshold_enable, lower_threshold_value);
		if (value >= 0)
			execute_callback(executable, value);
	}

	if (close(event_fd) == -1)
		perror("Failed to close event file");

error_free:
	free(dev_dir_name);

	return ret;
}
