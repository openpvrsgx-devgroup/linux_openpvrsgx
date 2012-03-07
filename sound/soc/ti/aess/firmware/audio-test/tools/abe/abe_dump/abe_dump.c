/*
 *  Copyright 2001-2011 Texas Instruments - http://www.ti.com/
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>

#define DMEM_ID		0x01
#define CMEM_ID		0x02
#define SMEM_ID		0x04
#define PMEM_ID		0x08
#define AESS_ID		0x10
#define ALL_ID		0x1F

#define DMEM_ADDR	0x49080000
#define DMEM_SIZE	(64 * 1024)
#define CMEM_ADDR	0x490A0000
#define CMEM_SIZE	(8 * 1024)
#define SMEM_ADDR	0x490C0000
#define SMEM_SIZE	(24 * 1024)
#define PMEM_ADDR	0x490E0000
#define PMEM_SIZE	(8 * 1024)
#define AESS_ADDR	0x490F1000
#define AESS_SIZE	(1 * 1024)

static void help(void)
{
	printf("Usage: abe_dump [OPTION]\n");
	printf("-d --dmem   Dump DMEM\n");
	printf("-c --cmem   Dump CMEM\n");
	printf("-s --smem   Dump SMEM\n");
	printf("-p --pmem   Dump PMEM\n");
	printf("-a --aess   Dump AESS registers\n");
	printf("-h --help   Help\n");
	printf("No option dumps all ABE mems\n");
}

static int dump_memory(char *filename, unsigned int baseaddr, unsigned int size)
{
	FILE *fp;
	void *base;
	unsigned int *ptr;
	int fd, i, ret = 0;

	fp = fopen(filename, "wb");
	if (fp == NULL) {
		printf("couldn't open file '%s'\n", filename);
		return 1;
	}

	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0) {
	    	printf("failed to open the mmap device\n");
		ret = -1;
		goto err1;
	}

	base = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, baseaddr);
	if (base < 0) {
		printf("failed to mmap\n");
		ret = -1;
		goto err2;
	}

	ptr = base;
	for (i = 0; i < size / 4; i++) {
		fprintf(fp, "0x%08x,\n", *ptr);
		ptr++;
	}

	ret = munmap(base, size);
	if (ret < 0)
		printf("failed to unmmap\n");


err2:
	close(fd);
err1:
	fclose(fp);
	return ret;
}

static const struct option long_options[] = {
	{"dmem", no_argument, 0, 'd'},
	{"cmem", no_argument, 0, 'c'},
	{"smem", no_argument, 0, 's'},
	{"pmem", no_argument, 0, 'p'},
	{"aess", no_argument, 0, 'a'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0},
};

static const char short_options[] = "dcspah";

int main(int argc, char *argv[])
{
	int dump = 0;
	signed char c;
	int ret;

	/* no args, dump all memories */
	if (argc == 1)
		dump = ALL_ID;

	while ((c = getopt_long(argc, argv, short_options, long_options, 0)) != -1) {
		switch (c) {
		case 'd':
			dump |= DMEM_ID;
			break;
		case 'c':
			dump |= CMEM_ID;
			break;
		case 's':
			dump |= SMEM_ID;
			break;
		case 'p':
			dump |= PMEM_ID;
			break;
		case 'a':
			dump |= AESS_ID;
			break;
		case 'h':
			help();
			return 0;
		default:
			help();
			return 1;
		};
	}

	if (dump & DMEM_ID) {
		ret = dump_memory("dmem.txt", DMEM_ADDR, DMEM_SIZE);
		if (ret) {
			printf("Failed to dump DMEM\n");
			return ret;
		}
	}

	if (dump & CMEM_ID) {
		ret = dump_memory("cmem.txt", CMEM_ADDR, CMEM_SIZE);
		if (ret) {
			printf("Failed to dump CMEM\n");
			return ret;
		}
	}


	if (dump & SMEM_ID) {
		ret = dump_memory("smem.txt", SMEM_ADDR, SMEM_SIZE);
		if (ret) {
			printf("Failed to dump SMEM\n");
			return ret;
		}
	}

	if (dump & PMEM_ID) {
		ret = dump_memory("pmem.txt", PMEM_ADDR, PMEM_SIZE);
		if (ret) {
			printf("Failed to dump PMEM\n");
			return ret;
		}
	}

	if (dump & AESS_ID) {
		ret = dump_memory("aess.txt", AESS_ADDR, AESS_SIZE);
		if (ret) {
			printf("Failed to dump AESS registers\n");
			return ret;
		}
	}

	return 0;
}
