
#include <abegen.h>

const uint32_t text[] = {
	#include "abe_firmware.c"
};

const struct abe_firmware fw = {
	.text	= text,
	.size 	= ARRAY_SIZE(text),
};
