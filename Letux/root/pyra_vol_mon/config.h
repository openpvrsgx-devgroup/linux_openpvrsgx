#pragma once

struct pyra_volume_config {
	unsigned int	channel;
	unsigned int	min;
	unsigned int	max;
	unsigned int	step;
};

int read_config_from_file(const char* filename, struct pyra_volume_config *cfg);
