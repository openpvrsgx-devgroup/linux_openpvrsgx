/*
 * Simple audio test capture to buffer
 *
 * Author: Liam Girdwood
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITH ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

#undef DEBUG

#define DEBUGFS	"/sys/kernel/debug/omap4-abe/debug"

static int complete = 0, wperiods = 0, rperiods = 0;

#ifdef DEBUG
#define dprintf(fmt, arg...)		printf(fmt, arg)
#else
#define dprintf(fmt, arg...)
#endif

struct trace {

	/* buffer data */
	char *buffer;
	int reader_position; /* in bytes */
	int writer_position;
	size_t buffer_size;
	size_t limit;
	int num_periods;
	int period_bytes;

	/* locking/threading */
	pthread_t tid;
	pthread_t pid;
	pthread_mutex_t mutex; /* for access to reader/writer pos */
	sem_t sem; /* for waking writer */
	sem_t rsem; /* for waking resume */

	/* file ops */
	int out_fd;
	int in_fd;
};

/* write PCM data to output file  - does not add WAV header */
static void *writer_thread(void *data)
{
	struct trace *trace = (struct trace*)data;
	char * buf = trace->buffer + trace->writer_position;
	int bytes, overlap, val;
	
	while (1) {
		/* wait for next frame to be avail */
		sem_wait(&trace->sem);

		if (complete == 1)
			goto out;

		/* pointer wrap ? */
		if (trace->writer_position == trace->period_bytes * trace->num_periods) {

			trace->writer_position = 0;
			buf = trace->buffer;
		}

		dprintf("%s: write 0x%x at offset 0x%x \n", __func__,
			trace->period_bytes, trace->writer_position);

		/* write whole period */
		bytes = write(trace->out_fd, buf, trace->period_bytes);
		wperiods++;

		/* inc writer pos */
		trace->writer_position += bytes;

		dprintf("%s: new write offset 0x%x \n", __func__,
			trace->writer_position);
		buf = trace->buffer + trace->writer_position;

		/* check how many periods are waiting to be written to file */
		sem_getvalue(&trace->sem, &val);

		/* has the writer been overtaken by the reader ??? */
		if (val >= trace->num_periods)
			printf("w: overwrite %d\n",val);
		if (val == 0 && complete == 1)
			goto out;
	}

out:
	printf("writer finished %d periods\n", wperiods);
	return NULL;
}

/* read tarec data from the audio driver */
static ssize_t trace_read(struct trace *trace, size_t count)
{
	char *data = trace->buffer + trace->reader_position;
	ssize_t size;
	ssize_t result = 0;
	int wait, val, overwrite = 0;
	
	/* check how many periods are free for reader */
	sem_getvalue(&trace->sem, &val);

	/* has we overtaken the writer ?? */
	if (val >= trace->num_periods - 1) {
		/*
		 * We have many choices here (depending on our policy):-
		 * 1. Restart capture.
		 * 2. Discard current period and wait for writer to catch up.
		 * 3. Discard/flush writer periods.
		 * We choose option 2 in this example.
		 */
		printf("r: overwrite %d\n",val);
		overwrite = 1;
	}

	/* read a number of frames from the driver */
	while (count > 0) {

		size = read(trace->in_fd, data, count);
		if (size < 0) {
			printf("read error on fd %d for %d bytes: %d\n", trace->in_fd, count, size);
			complete = 1;
			sem_post(&trace->sem);
			return result;
		}
		
		/* still have data to read ? */
		if (size > 0) {
			result += size;
			count -= size;
			data += size;
		}
	}

	/* return if we are about to overwrrite */
	if (overwrite)
		return result;
	
	/* got frames, so update reader pointer */
	trace->reader_position += result;
	dprintf("%s: buffer pos 0x%x size 0x%x\n", __func__, trace->reader_position,
		trace->period_bytes * trace->num_periods);

	/* buffer wrap ? */
	if (trace->reader_position >= trace->period_bytes * trace->num_periods)
		trace->reader_position = 0;

	/* tel writer we have frame */
	sem_post(&trace->sem);
	rperiods++;

	return result;
}

/* CTRL - C handler - hit twice to really kill, let the write catch up. */
static void shutdown(int sig)
{
	if (complete)
		exit(0);

	complete = 1;
}

/* Todo - driver option formats currently not set */
static const struct option long_options[] = {
	{"buffer", 1, 0, 'b'},
	{"limit", 1, 0, 'l'},
}; 

static const char short_options[] = "b:l:";

int main(int argc, char *argv[])
{
	struct trace *trace;
	int ret, not_ready = 1, val, option = 0, i, time;
	char *out_file, c;

	trace = calloc(1, sizeof(struct trace));
	if (trace == NULL)
		return -ENOMEM;

	trace->period_bytes = 128 * 8 * 16;
	trace->num_periods = 4;

#if 0
	while ((c = getopt_long(argc, argv, short_options, long_options, &option)) != -1) {
		switch (c) {
		case 'b': /* userspace buffer size */
			trace->buffer_size = strtol(optarg, NULL, 0);
			break;
		case 'l': /* trace capture limit */
			trace->limit = strtol(optarg, NULL, 0);
			break;
		default:
			printf("%s invalid option\n", argv[0]);
			return 1;
		}
	}
#endif
	
	trace->buffer = malloc(trace->period_bytes * trace->num_periods);
	if (trace->buffer == NULL)
		return -ENOMEM;

	if (argc > option) {
		out_file = argv[optind];
	} else {
		printf(" no trace capture file specified\n");
		exit(-EINVAL);
	}
	
	/* catch ctrl C and dump buffer info */
	signal(SIGINT, shutdown);

	/* the output file */
	trace->out_fd = open(out_file, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (trace->out_fd < 0) {
		printf("failed to open %s err %d\n",
			out_file, trace->out_fd);
		return trace->out_fd;
	}

	/* open the trace source */
	trace->in_fd = open(DEBUGFS, O_RDONLY);
	if (trace->in_fd < 0) {
		printf("failed to open %s err %d\n", DEBUGFS, trace->in_fd);
		return trace->out_fd;
	}
	
	/* init threads */
	pthread_mutex_init(&trace->mutex, NULL);
	sem_init(&trace->sem, 0, 0);
	sem_init(&trace->rsem, 0, 0);
	ret = pthread_create(&trace->tid, NULL, writer_thread, trace);
	if (ret < 0) {
		printf("failed to create thread %d\n", ret);
		return ret;
	}

	/* todo - exit the loop after n iterations */
	while (!complete)
		trace_read(trace, trace->period_bytes);
	
	/* this may be different from writer periods depending where in
	   the read/write cycle ctrl-C was pressed */
	printf("reader finished %d periods\n", rperiods);

	/* wait for writer to finish */
	pthread_join(trace->tid, NULL);

	/* cleanup */
	close(trace->in_fd);
	close(trace->out_fd);

	return 0;
}


