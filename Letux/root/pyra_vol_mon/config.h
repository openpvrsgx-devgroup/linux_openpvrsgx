#pragma once

struct pyra_volume_config {
	unsigned int	channel;
	unsigned int	min;
	unsigned int	max;
	unsigned int	step;
	const char*	executable;
};

int pyra_get_config(struct pyra_volume_config *cfg, int argc, char *argv[]);
