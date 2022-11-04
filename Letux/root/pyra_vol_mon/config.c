#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_CHANNEL	(2)
#define DEFAULT_MIN	(0)
#define DEFAULT_MAX	(0x7FF)
#define DEFAULT_STEP	(25)

static int parse_number(const char* str)
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

int pyra_get_config(struct pyra_volume_config *cfg, int argc, char *argv[])
{
	cfg->channel = DEFAULT_CHANNEL;
	cfg->min = DEFAULT_MIN;
	cfg->max = DEFAULT_MAX;
	cfg->step = DEFAULT_STEP;
	cfg->executable = argv[1];

	return 0;
}
