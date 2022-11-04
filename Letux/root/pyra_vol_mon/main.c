#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/iio/events.h>
#include <linux/iio/types.h>

#include "pyra_vol_mon.h"

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
	struct pyra_iio_event_handle *iio_event_handle;

	ret = pyra_get_config(&config, argc, argv);
	if (ret < 0)
		return ret;

	event_fd = pyra_iio_event_open(&iio_event_handle, config.channel);
	if (event_fd < 0)
		return event_fd;

	ret = read_value_and_update_thresholds(&config, iio_event_handle);
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

		value = read_value_and_update_thresholds(&config, iio_event_handle);
		if (value >= 0)
			execute_callback(config.executable, value);
	}

	if (close(event_fd) == -1)
		perror("Failed to close event file");

	pyra_iio_event_free(iio_event_handle);

	return ret;
}
