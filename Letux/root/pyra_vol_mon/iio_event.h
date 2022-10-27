#pragma once

struct pyra_iio_event_handle {
	const char *dev_dir_name;
	const char *input;
	const char *upper_enable;
	const char *upper_threshold;
	const char *lower_enable;
	const char *lower_threshold;
};

int pyra_iio_event_open(struct pyra_iio_event_handle* handle, int channel);
void pyra_iio_event_free(struct pyra_iio_event_handle* handle);
