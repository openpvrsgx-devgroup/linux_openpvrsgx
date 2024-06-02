#include <stdio.h>
#include <errno.h>

#include "pyra_vol_mon.h"

int read_value_and_update_thresholds(
		const struct pyra_volume_config *config,
		const struct pyra_iio_event_handle *iio)
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

	/* clamp value to [min, max] */
	if (value > config->max)
		value = config->max;
	else if (value < config->min)
		value = config->min;

	fprintf(stderr, "value %d threshold %d min %d max %d\n", value, threshold, config->min, config->max);

	/* update upper threshold */
	if (value == config->max) {
		ret = pyra_iio_disable_upper_threshold(iio);
		if (ret < 0)
			fprintf(stderr, "Failed to disable upper threshold %d: %d\n", threshold, ret);
	} else {

		threshold = value + config->step;
		if (threshold > (int)config->max)
			threshold = config->max - 1;	// set to last step

		ret = pyra_iio_enable_upper_threshold(iio, threshold);
		if (ret < 0)
			fprintf(stderr, "Failed to enable upper threshold %d: %d\n", threshold, ret);
	}

	/* update lower threshold */
	if (value == config->min) {
		ret = pyra_iio_disable_lower_threshold(iio);
		if (ret < 0)
			fprintf(stderr, "Failed to disable lower threshold %d: %d\n", threshold, ret);
	} else {

		threshold = value - config->step;
		if (threshold < (int)config->min)
			threshold = config->min + 1;	// set to last step

		ret = pyra_iio_enable_lower_threshold(iio, threshold);
		if (ret < 0)
			fprintf(stderr, "Failed to enable lower threshold %d: %d\n", threshold, ret);
	}

	return value;
}
