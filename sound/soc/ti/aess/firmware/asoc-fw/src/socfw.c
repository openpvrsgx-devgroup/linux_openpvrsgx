/*

  Copyright(c) 2010-2011 Texas Instruments Incorporated,
  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.
*/

#define uintptr_t host_uintptr_t
#define mode_t host_mode_t
#define dev_t host_dev_t
#define blkcnt_t host_blkcnt_t
typedef int int32_t;

#define _SYS_TYPES_H 1
#include <stdlib.h>
#undef __always_inline
#undef __extern_always_inline
#undef __attribute_const__
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "socfw.h"

struct soc_fw_priv;

struct soc_fw_priv *socfw_new(const char *name, int verbose);
void socfw_free(struct soc_fw_priv *soc_fw);
int socfw_import_plugin(struct soc_fw_priv *soc_fw, const char *name);
int socfw_import_vendor(struct soc_fw_priv *soc_fw, const char *name, int type);
int socfw_import_dapm_graph(struct soc_fw_priv *soc_fw,
	const struct snd_soc_dapm_route *graph, int graph_count);
int socfw_import_dapm_widgets(struct soc_fw_priv *soc_fw,
	const struct snd_soc_dapm_widget *widgets, int widget_count);
int socfw_import_controls(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrols, int kcontrol_count);

static void usage(char *name)
{
	fprintf(stdout, "usage: %s outfile [options]\n\n", name);

	fprintf(stdout, "Add plugin data		[-p plugin]\n");
	fprintf(stdout, "Add controls			[-c controls]\n");
	fprintf(stdout, "Add vendor firmware		[-vfw firmware]\n");
	fprintf(stdout, "Add vendor coefficients 	[-vcf coefficients]\n");
	fprintf(stdout, "Add vendor configuration	[-vcn config]\n");
	fprintf(stdout, "Add vendor codec		[-vcd codec]\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	struct soc_fw_priv *soc_fw;
	int i;
	
	if (argc < 4)
		usage(argv[0]);

	setvbuf(stdout, NULL, _IONBF, 0);

	soc_fw = socfw_new(argv[1], 1);
	if (soc_fw < 0) {
		fprintf(stderr, "failed to open %s\n", argv[argc - 1]);
		exit(1);
	}

	for (i = 2 ; i < argc - 1; i++) {

		/* plugin - kcontrols, DAPM graph, widgets, pins, coeffcients etc */
		if (!strcmp("-p", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

			socfw_import_plugin(soc_fw, argv[i]);
			continue;
		}

		/* vendor options */
		if (!strcmp("-vfw", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

			socfw_import_vendor(soc_fw, argv[i], SND_SOC_TPLG_TYPE_VENDOR_FW);
			continue;
		}
		if (!strcmp("-vcf", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

			socfw_import_vendor(soc_fw, argv[i], SND_SOC_TPLG_TYPE_VENDOR_CONFIG);
			continue;
		}
		if (!strcmp("-vco", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

			socfw_import_vendor(soc_fw, argv[i], SND_SOC_TPLG_TYPE_VENDOR_COEFF);
			continue;
		}
		if (!strcmp("-vcn", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

// NOTE: missing _ typo in include/uapi/sound/asoc.h
			socfw_import_vendor(soc_fw, argv[i], SND_SOC_TPLG_TYPEVENDOR_CODEC);
			continue;
		}
	}

	socfw_free(soc_fw);
	return 0;
}

