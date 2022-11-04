#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <linux/iio/events.h>
#include <linux/iio/types.h>

#include "config.h"
#include "iio_event.h"

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

static int read_value_and_update_thresholds(
		struct pyra_volume_config *config,
		struct pyra_iio_event_handle *iio)
{
	int ret;
	int value;
	int threshold;

	ret = pyra_iio_get_value(iio);
	if (ret < 0) {
		fprintf(stderr, "Error reading current value: %d\n", ret);
		return -EAGAIN;
	}
	
	value = ret;

	/* update upper threshold */
	threshold = value + config->step;
	if (threshold > config->max)
		threshold = config->max;

	if (threshold > value) {
		ret = pyra_iio_enable_upper_threshold(iio, threshold);
		if (ret)
			fprintf(stderr, "Failed to enable upper threshold: %d\n", ret);
	}
	else {
		ret = pyra_iio_disable_upper_threshold(iio);
		if (ret < 0)
			fprintf(stderr, "Failed to disable upper threshold: %d\n", ret);
	}

	/* update lower threshold */
	threshold = value - config->step;
	if (threshold < (int)config->min)
		threshold = config->min;

	/* avoid enabling a threshold that's below the step value */
	if (threshold < value && threshold > config->step) {
		ret = pyra_iio_enable_lower_threshold(iio, threshold);
		if (ret < 0)
			fprintf(stderr, "Failed to enable lower threshold: %d\n", ret);
	}
	else {
		ret = pyra_iio_disable_lower_threshold(iio);
		if (ret < 0)
			fprintf(stderr, "Failed to disable lower threshold: %d\n", ret);
	}

	return value;
}

static int execute_callback(const char* prog, int value)
{
	pid_t my_pid;
	char value_string[32];

	snprintf(value_string, sizeof(value_string), "%d", value);

	if (0 == (my_pid = fork())) {
		if (-1 == execlp(prog, basename(prog), value_string, 0)) {
			perror("child process execve failed");
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct iio_event_data event;
	int ret;
	int event_fd;
	struct pyra_volume_config config;
	struct pyra_iio_event_handle iio_event_handle;

	ret = pyra_get_config(&config, argc, argv);
	if (ret < 0)
		return ret;

	event_fd = pyra_iio_event_open(&iio_event_handle, config.channel);
	if (event_fd < 0)
		return event_fd;

	ret = read_value_and_update_thresholds(&config, &iio_event_handle);
	if (ret >= 0)
		execute_callback(config.executable, ret);

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

		if (!event_is_ours(&event, config.channel))
			continue;

		value = read_value_and_update_thresholds(&config, &iio_event_handle);
		if (value >= 0)
			execute_callback(config.executable, value);
	}

	if (close(event_fd) == -1)
		perror("Failed to close event file");

	pyra_iio_event_free(&iio_event_handle);

	return ret;
}
