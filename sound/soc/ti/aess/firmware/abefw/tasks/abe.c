
/* Include path set by Makefile depending on version being built */
#include <abe_addr.c>

const struct omap_aess_mapping aess_fw_init = {
	.map 		= omap_aess_map,
	.map_count 	= ARRAY_SIZE(omap_aess_map),

	.fct_id		= omap_function_id,
	.fct_count	= ARRAY_SIZE(omap_function_id),

	.label_id	= omap_label_id,
	.label_count	= ARRAY_SIZE(omap_label_id),

	.init_table	= init_table,
	.table_count	= ARRAY_SIZE(init_table),

	.port		= abe_port_init,
	.port_count	= ARRAY_SIZE(abe_port_init),

	.ping_pong	= &abe_port_init_pp,
	.dl1_mono_mixer	= aess_dl1_mono_mixer,
	.dl2_mono_mixer	= aess_dl2_mono_mixer,
	.audul_mono_mixer	= aess_audul_mono_mixer,
};
