#pragma once

#include "config.h"
#include "iio_event.h"

int read_value_and_update_thresholds(
		struct pyra_volume_config *config,
		struct pyra_iio_event_handle *iio);
