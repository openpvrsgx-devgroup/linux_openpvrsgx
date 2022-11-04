#pragma once

#include "config.h"
#include "iio_event.h"

int read_value_and_update_thresholds(
		const struct pyra_volume_config *config,
		const struct pyra_iio_event_handle *iio);
