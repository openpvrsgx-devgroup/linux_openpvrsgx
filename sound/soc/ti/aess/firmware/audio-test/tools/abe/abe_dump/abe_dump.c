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

static void help(char *app_name)
{
	printf("%s: [DMEM] [CMEM] [SMEM] [PMEM] [AESS]\n", app_name);
	printf("Alias: all, safe (no PMEM)\n");
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

int main(int argc, char **argv)
{
	int dmem = 0, cmem = 0, smem = 0, pmem = 0, aess = 0, i;

	if (argc < 2) {
		printf("No memory specified\n");
		help(argv[0]);
	}

	for (i = 1; i < argc; i++) {
		if (strstr("DMEM", argv[i]))
			dmem = 1;
		else if (strstr("CMEM", argv[i]))
			cmem = 1;
		else if (strstr("SMEM", argv[i]))
			smem = 1;
		else if (strstr("PMEM", argv[i]))
			pmem = 1;
		else if (strstr("AESS", argv[i]))
			aess = 1;
		else if (strstr("all", argv[i]))
			dmem = cmem = smem = pmem = aess = 1;
		else if (strstr("safe", argv[i]))
			dmem = cmem = smem = aess = 1;
		else
			printf("invalid argument '%s', skipping\n", argv[i]);
	}

	if (dmem)
		dump_memory("dmem.txt", 0x49080000, 64 * 1024);
	if (cmem)
		dump_memory("cmem.txt", 0x490A0000, 8 * 1024);
	if (smem)
		dump_memory("smem.txt", 0x490C0000, 24 * 1024);
	if (pmem)
		dump_memory("pmem.txt", 0x490E0000, 8 * 1024);
	if (aess)
		dump_memory("aess.txt", 0x490F1000, 1 * 1024);

	return 0;
}
