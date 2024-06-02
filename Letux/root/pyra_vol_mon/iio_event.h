#pragma once

struct pyra_iio_event_handle;

/* init and deinit */
int pyra_iio_event_open(struct pyra_iio_event_handle** handle, int channel);
void pyra_iio_event_free(struct pyra_iio_event_handle* handle);
/* current value */
int pyra_iio_get_value(const struct pyra_iio_event_handle* handle);
/* upper threshold */
int pyra_iio_enable_upper_threshold(const struct pyra_iio_event_handle* handle, int threshold);
int pyra_iio_disable_upper_threshold(const struct pyra_iio_event_handle* handle);
/* lower threshold */
int pyra_iio_enable_lower_threshold(const struct pyra_iio_event_handle* handle, int threshold);
int pyra_iio_disable_lower_threshold(const struct pyra_iio_event_handle* handle);
