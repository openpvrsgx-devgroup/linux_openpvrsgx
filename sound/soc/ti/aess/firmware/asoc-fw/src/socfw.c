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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "socfw.h"

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

	soc_fw = socfw_new(argv[1], 1);
	if (soc_fw < 0) {
		fprintf(stderr, "failed to open %s\n", argv[argc - 1]);
		exit(0);
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

			socfw_import_vendor(soc_fw, argv[i], SND_SOC_FILE_VENDOR_FW);
			continue;
		}
		if (!strcmp("-vcf", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

			socfw_import_vendor(soc_fw, argv[i], SND_SOC_FILE_VENDOR_COEFF);
			continue;
		}
		if (!strcmp("-vco", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

			socfw_import_vendor(soc_fw, argv[i], SND_SOC_FILE_VENDOR_CODEC);
			continue;
		}
		if (!strcmp("-vcn", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

			socfw_import_vendor(soc_fw, argv[i], SND_SOC_FILE_VENDOR_CONFIG);
			continue;
		}
	}

	socfw_free(soc_fw);
	return 0;
}

