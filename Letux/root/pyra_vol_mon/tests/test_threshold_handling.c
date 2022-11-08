#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pyra_vol_mon.h"

#define RUN_TEST(test) do {\
	printf("Running " #test " - "); \
	fflush(stdout); \
	ret = test(); \
	if (ret) \
		return -1; \
	printf("PASS\n"); \
} while (0)
#define ASSERT(cond) do {\
		if (!(cond)) { \
			printf("\n\t%s:%d: Assertion failed: " #cond "\n", __func__, __LINE__); \
			return -1; \
		} \
	} while (0)

// same as in iio_event.c
struct pyra_iio_event_handle {
	const char *dev_dir_name;
	const char *input;
	const char *upper_enable;
	const char *upper_threshold;
	const char *lower_enable;
	const char *lower_threshold;
};

static const struct pyra_iio_event_handle g_iio = {
	.dev_dir_name		= "test",
	.input			= "input",
	.upper_enable		= "up_en",
	.upper_threshold	= "up_th",
	.lower_enable		= "lo_en",
	.lower_threshold	= "lo_th",
};

static struct pyra_volume_config g_config = {
	.channel	= 2,
	.min		= 0,
	.max		= 0x7FF,
	.step		= 10,
	.executable	= "echo",
};

const char *iio_dir = "/tmp/";

/*
 * FAKES
 */
static int g_input_value;
static int g_upper_threshold;
static int g_lower_threshold;
static bool g_upper_enable;
static bool g_lower_enable;

int read_sysfs_posint(const char *filename, const char *basedir)
{
	if (strcmp(basedir, g_iio.dev_dir_name))
		return -1;
	if (strcmp(filename, g_iio.input))
		return -1;
	return g_input_value;
}

int write_sysfs_int_and_verify(const char *filename, const char *basedir, int val)
{
	if (strcmp(basedir, g_iio.dev_dir_name))
		return -1;

	if (!strcmp(filename, g_iio.input))
		return -1; // can't write to "input" file
	else if (!strcmp(filename, g_iio.upper_enable)) {
		g_upper_enable = val;
		return val;
	}
	else if (!strcmp(filename, g_iio.upper_threshold)) {
		g_upper_threshold = val;
		return val;
	}
	else if (!strcmp(filename, g_iio.lower_enable)) {
		g_lower_enable = val;
		return val;
	}
	else if (!strcmp(filename, g_iio.lower_threshold)) {
		g_lower_threshold = val;
		return val;
	}

	return -1;
}

/*
 * STUBS
 */
int find_type_by_name()
{
	return -1;
}

/*
 * TESTS
 */
static int test_disable_lower_threshold(void)
{
	int ret;

	g_config.step = 10;
	g_config.min = 10;

	g_upper_enable = false;
	g_upper_threshold = 0;
	g_lower_enable = true;
	g_lower_threshold = 100;

	g_input_value = 10;

	ret = read_value_and_update_thresholds(&g_config, &g_iio);
	ASSERT(ret == g_input_value);
	ASSERT(g_upper_enable);
	ASSERT(g_upper_threshold == (g_input_value + g_config.step));
	ASSERT(!g_lower_enable);

	g_config.min = 0;

	g_upper_enable = false;
	g_upper_threshold = 0;
	g_lower_enable = true;

	g_input_value = 10;

	ret = read_value_and_update_thresholds(&g_config, &g_iio);

	ASSERT(ret == g_input_value);
	ASSERT(g_upper_enable);
	ASSERT(g_upper_threshold == (g_input_value + g_config.step));
	ASSERT(!g_lower_enable);

	return 0;
}

static int test_disable_upper_threshold(void)
{
	int ret;

	g_config.step = 10;
	g_config.max = 500;

	g_upper_enable = true;
	g_lower_enable = false;
	g_lower_threshold = 0;

	g_input_value = 500;

	ret = read_value_and_update_thresholds(&g_config, &g_iio);
	ASSERT(ret == g_input_value);
	ASSERT(!g_upper_enable);
	ASSERT(g_lower_enable);
	ASSERT(g_lower_threshold == (g_input_value - g_config.step));

	return 0;
}

static int test_enable_both_thresholds(void)
{
	int ret;

	g_config.step = 10;

	g_upper_enable = false;
	g_lower_enable = false;
	g_upper_threshold = 0;
	g_lower_threshold = 0;

	g_input_value = 100;

	ret = read_value_and_update_thresholds(&g_config, &g_iio);
	ASSERT(ret == g_input_value);
	ASSERT(g_upper_enable);
	ASSERT(g_upper_threshold == (g_input_value + g_config.step));
	ASSERT(g_lower_enable);
	ASSERT(g_lower_threshold == (g_input_value - g_config.step));

	return 0;
}

static int test_clamp_lower_threshold(void)
{
	int ret;

	g_config.step = 10;
	g_config.min = 50;

	g_lower_enable = false;
	g_lower_threshold = 0;
	g_upper_enable = true;
	g_upper_threshold = 100;

	g_input_value = 55;

	ret = read_value_and_update_thresholds(&g_config, &g_iio);
	ASSERT(ret == g_input_value);
	ASSERT(g_lower_enable);
	ASSERT(g_lower_threshold == g_config.min);
	ASSERT(g_upper_enable);
	ASSERT(g_upper_threshold == (g_input_value + g_config.step));

	return 0;
}

static int test_clamp_upper_threshold(void)
{
	int ret;

	g_config.step = 10;
	g_config.min = 0;
	g_config.max = 500;

	g_lower_enable = false;
	g_lower_threshold = 0;
	g_upper_enable = false;
	g_upper_threshold = 100;

	g_input_value = 495;

	ret = read_value_and_update_thresholds(&g_config, &g_iio);
	ASSERT(ret == g_input_value);
	ASSERT(g_lower_enable);
	ASSERT(g_lower_threshold == (g_input_value - g_config.step));
	ASSERT(g_upper_enable);
	ASSERT(g_upper_threshold == g_config.max);

	return 0;
}

static int test_abort_on_bad_input(void)
{
	int ret;

	g_input_value = -1;

	g_upper_enable = true;
	g_upper_threshold = 110;
	g_lower_enable = true;
	g_lower_threshold = 90;

	ret = read_value_and_update_thresholds(&g_config, &g_iio);
	ASSERT(ret < 0);
	ASSERT(g_upper_enable);
	ASSERT(g_lower_enable);
	ASSERT(g_upper_threshold == 110);
	ASSERT(g_lower_threshold == 90);

	return 0;
}

int main(int argc, char* argv[])
{
	int ret;

	// suppress error printouts
	fclose(stderr);

	RUN_TEST(test_disable_lower_threshold);
	RUN_TEST(test_disable_upper_threshold);
	RUN_TEST(test_enable_both_thresholds);
	RUN_TEST(test_clamp_lower_threshold);
	RUN_TEST(test_clamp_upper_threshold);
	RUN_TEST(test_abort_on_bad_input);

	printf("All tests passed!\n");
	return 0;
}
