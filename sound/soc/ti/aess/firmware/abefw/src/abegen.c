/*
  BSD LICENSE

  Copyright(c) 2010-2011 Texas Instruments Incorporated,
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Texas Instruments Incorporated nor the names of
      its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "abegen.h"
#include "abe-local.h"

int (*omap_aess_init_asrc_vx_dl)(s32 *el, s32 dppm);
int (*omap_aess_init_asrc_vx_ul)(s32 *el, s32 dppm);

static int abe_dlopen_fw(const char *fw_version)
{
	void *plugin_handle;
	struct abe_firmware *fw;
	int ret, fd;

	fprintf(stdout, "FW: loading %s\n", fw_version);

	/* load the plugin */
	plugin_handle = dlopen(fw_version, RTLD_LAZY);
	if (!plugin_handle) {
		fprintf(stderr, "error: failed to open %s: %s\n", fw_version,
			dlerror());
		return -EINVAL;
	}

	fw = dlsym(plugin_handle, "fw");
	if (!fw) {
		fprintf(stderr, "error: failed to get symbol. %s\n",
			dlerror());
		dlclose(plugin_handle);
		return -EINVAL;
	}

	/* dump some plugin info */
	fprintf(stdout, "FW: loaded %d bytes\n", fw->size);
	
	/* save data to FW file */
	fd = open("omap_abe_fw", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s err %d\n", "omap_abe_fw", fd);
		ret = fd;
		goto out;
	}

	ret = write(fd, fw->text, fw->size * 4);
	close(fd);

out:
	dlclose(plugin_handle);
	return ret;
}

static int mwrite(int fd, const void *buf, int size)
{
	int ret;

	ret = write(fd, buf, size);
	if (ret < 0) {
		fprintf(stderr, "failed to write %d bytes\n", size);
		exit(ret);
	}
	return ret / 4;
}

static int abe_task_gen(struct omap_aess_mapping *m, int fd)
{
	s32 data_asrc[55];
	int offset = 0, i;

	/* write map */
	fprintf(stdout, "Mem map: %d entries of size %ld at offset %d bytes %d\n",
		m->map_count, sizeof(*m->map), offset, offset * 4);
	offset += mwrite(fd, &m->map_count, sizeof(m->map_count));
	offset += mwrite(fd, m->map, sizeof(struct omap_aess_addr) * m->map_count);

	/* write label ids */
	fprintf(stdout, "Label: %d entries of size %ld at offset %d bytes %d\n",
		m->label_count, sizeof(*m->label_id), offset, offset * 4);
	offset += mwrite(fd, &m->label_count, sizeof(m->label_count));
	offset += mwrite(fd, m->label_id, sizeof(*m->label_id) * m->label_count);

	/* write function ids */
	fprintf(stdout, "Functions: %d entries of size %ld at offset %d bytes %d\n",
		m->fct_count, sizeof(*m->fct_id), offset, offset * 4);
	offset += mwrite(fd, &m->fct_count, sizeof(m->fct_count));
	offset += mwrite(fd, m->fct_id, sizeof(*m->fct_id) * m->fct_count);

	/* write tasks */
	fprintf(stdout, "Task: %d entries of size %ld at offset %d bytes %d\n",
		m->table_count, sizeof(*m->init_table), offset, offset * 4);
	offset += mwrite(fd, &m->table_count, sizeof(m->table_count));
	offset += mwrite(fd, m->init_table, sizeof(*m->init_table) * m->table_count);

	/* write ports */
	fprintf(stdout, "Port: %d entries of size %ld at offset %d bytes %d\n",
		m->port_count, sizeof(*m->port), offset, offset * 4);
	offset += mwrite(fd, &m->port_count, sizeof(m->port_count));
	offset += mwrite(fd, m->port, sizeof(*m->port) * m->port_count);

	/* ping pong port */
	fprintf(stdout, "Ping Pong port: at offset %d bytes %d\n",
		offset, offset * 4);
	offset += mwrite(fd, m->ping_pong, sizeof(*m->ping_pong));	

	/* DL1 port */
	fprintf(stdout, "DL1 port: at offset %d offset %d\n",
		offset, offset * 4);
	offset += mwrite(fd, m->dl1_mono_mixer, sizeof(*m->dl1_mono_mixer));

	/* DL2 port */
	fprintf(stdout, "DL2 port: at offset %d bytes %d\n",
		offset, offset * 4);
	offset += mwrite(fd, m->dl2_mono_mixer, sizeof(*m->dl2_mono_mixer));

	/* AUDUL port */
	fprintf(stdout, "AUDUL port: at offset %d bytes %d\n",
		offset, offset * 4);
	offset += mwrite(fd, m->audul_mono_mixer, sizeof(*m->audul_mono_mixer));

	/* Voice UL ASRC */
	i = omap_aess_init_asrc_vx_ul(&data_asrc[0], 0);
	fprintf(stdout,"ASRC UL: %d at offset %d\n", i, offset);
	offset += mwrite(fd, &data_asrc[0], sizeof(s32)*i);
	i = omap_aess_init_asrc_vx_ul(&data_asrc[0], -250);
	fprintf(stdout,"ASRC UL(-250): %d at %d\n", i, offset);
	offset += mwrite(fd, &data_asrc[0], sizeof(s32)*i);

	/* Voice DL ASRC */
	i = omap_aess_init_asrc_vx_dl(&data_asrc[0], 0);
	fprintf(stdout,"ASRC DL: %d at %d\n", i, offset);
	offset += mwrite(fd, &data_asrc[0], sizeof(s32)*i);
	fprintf(stdout,"ASRC DL (250): %d at %d\n", i, offset);
	i = omap_aess_init_asrc_vx_dl(&data_asrc[0], 250);
	offset += mwrite(fd, &data_asrc[0], sizeof(s32)*i);

	fprintf(stdout,"Size of ABE configuration: %d bytes\n", offset * 4);
	return 0;
}

static int abe_dlopen_tasks(const char *tasks_version)
{
	void *plugin_handle;
	struct omap_aess_mapping *mapping;
	int ret, fd;

	fprintf(stdout, "Tasks: loading %s\n", tasks_version);

	/* load the plugin */
	plugin_handle = dlopen(tasks_version, RTLD_LAZY);
	if (!plugin_handle) {
		fprintf(stderr, "error: failed to open %s: %s\n", tasks_version,
			dlerror());
		return -EINVAL;
	}

	mapping = dlsym(plugin_handle, "aess_fw_init");
	if (!mapping) {
		fprintf(stderr, "error: failed to get symbol. %s\n",
			dlerror());
		dlclose(plugin_handle);
		return -EINVAL;
	}

	omap_aess_init_asrc_vx_dl = dlsym(plugin_handle, "omap_aess_init_asrc_vx_dl");
	if (!omap_aess_init_asrc_vx_dl) {
		fprintf(stderr, "error: failed to get omap_aess_init_asrc_vx_dl. %s\n",
			dlerror());
		dlclose(plugin_handle);
		return -EINVAL;
	}

	omap_aess_init_asrc_vx_ul = dlsym(plugin_handle, "omap_aess_init_asrc_vx_ul");
	if (!omap_aess_init_asrc_vx_ul) {
		fprintf(stderr, "error: failed to get omap_aess_init_asrc_vx_ul. %s\n",
			dlerror());
		dlclose(plugin_handle);
		return -EINVAL;
	}

	/* save data to FW file */
	fd = open("omap_abe_map", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s err %d\n", "omap_abe_map", fd);
		ret = fd;
		goto out;
	}

	ret = abe_task_gen(mapping, fd);
	close(fd);

out:
	dlclose(plugin_handle);
	return ret;
}

static void usage(char *name)
{
	fprintf(stdout, "usage: %s [options]\n\n", name);

	fprintf(stdout, "Create firmware		[-f version]\n");
	fprintf(stdout, "Create tasks			[-t version]\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int i;
	
	if (argc < 2)
		usage(argv[0]);

	for (i = 1 ; i < argc - 1; i++) {

		/* FW */
		if (!strcmp("-f", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

			abe_dlopen_fw(argv[i]);
			continue;
		}

		/* Tasks */
		if (!strcmp("-t", argv[i])) {
			if (++i == argc)
				usage(argv[0]);

			abe_dlopen_tasks(argv[i]);
			continue;
		}
	}

	return 0;
}
