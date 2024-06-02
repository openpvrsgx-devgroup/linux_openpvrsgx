/*
 * Simple audio test play from buffer
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

#include <alsa/asoundlib.h>
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

#define DEBUG

#ifdef DEBUG
#define dprintf(fmt, arg...)		printf(fmt, arg)
#else
#define dprintf(fmt, arg...)
#endif

static int complete = 0, wperiods = 0, rperiods = 0, pcm_pause = 0, irq = 0;

struct alsa_source {
	snd_pcm_t *handle;
	
	/* from cmd line parameters */
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
	unsigned int period_bytes;  /* bytes in a period */
	unsigned int num_periods;
	unsigned int num_dperiods;
	
	/* derived from cmd line params */
	snd_pcm_uframes_t period_frames; /* frames in a period */
	int bits_per_sample;
	int bits_per_frame;
	
	/* from sound hardware */
	size_t buffer_size;
	snd_pcm_uframes_t buffer_frames;  /* size of buffer in frames */

	/* buffer data */
	char *buffer;
	int reader_position; /* in bytes */
	int writer_position;

	/* locking/threading */
	pthread_t tid;
	pthread_t pid;
	pthread_mutex_t mutex; /* for access to reader/writer pos */
	sem_t sem; /* for waking writer */
	sem_t rsem; /* for waking resume */

	/* out file */
	int in_fd;

	/* debug buffer utilisation */
	int *debug_avail_per;
};

/* for debug logging */
static snd_output_t *log;

static int dump_control(snd_ctl_t *handle, snd_ctl_elem_id_t *id)
{
	int err, count, i;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_type_t type;
	snd_ctl_elem_value_t *control;

	/* alloc mem */
	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_value_alloca(&control);

	/* get info for control */
	snd_ctl_elem_info_set_id(info, id);
	err = snd_ctl_elem_info(handle, info);
	if (err < 0) {
		printf("failed to get ctl info\n");
		return err;
	}

	/* read the control */
	snd_ctl_elem_value_set_id(control, id);
	snd_ctl_elem_read(handle, control);

	/* get type and channels for control */
	type = snd_ctl_elem_info_get_type(info);
	count = snd_ctl_elem_info_get_count(info);
	if (count == 0)
		return 0;

	printf("%u:'%s':%d:",
	       snd_ctl_elem_id_get_numid(id),
	       snd_ctl_elem_id_get_name(id), count);

	switch (type) {
	case SND_CTL_ELEM_TYPE_BOOLEAN:
		for (i = 0; i < count - 1; i++)
			printf("%d,",
				snd_ctl_elem_value_get_boolean(control, i));
		printf("%d", snd_ctl_elem_value_get_boolean(control, i));
		break;
	case SND_CTL_ELEM_TYPE_INTEGER:
		for (i = 0; i < count - 1; i++)
			printf("%ld,",
				snd_ctl_elem_value_get_integer(control, i));
		printf("%ld", snd_ctl_elem_value_get_integer(control, i));
		break;
	case SND_CTL_ELEM_TYPE_INTEGER64:
		for (i = 0; i < count - 1; i++)
			printf("%lld,",
				snd_ctl_elem_value_get_integer64(control, i));
		printf("%lld",
				snd_ctl_elem_value_get_integer64(control, i));
		break;
	case SND_CTL_ELEM_TYPE_ENUMERATED:
		for (i = 0; i < count - 1; i++)
			printf("%d,",
				snd_ctl_elem_value_get_enumerated(control, i));
		printf("%d",
				snd_ctl_elem_value_get_enumerated(control, i));
		break;
	case SND_CTL_ELEM_TYPE_BYTES:
		for (i = 0; i < count - 1; i++)
			printf("%2.2x,",
				snd_ctl_elem_value_get_byte(control, i));
		printf("%2.2x", snd_ctl_elem_value_get_byte(control, i));
		break;
	default:
		break;
	}
	printf("\n");
	return 0;
}


static int mixers_display(snd_output_t *output, const char *card_name)
{
	snd_ctl_t *handle;
	snd_ctl_card_info_t *info;
	snd_ctl_elem_list_t *list;
	int ret, i, count;

	snd_ctl_card_info_alloca(&info);
	snd_ctl_elem_list_alloca(&list);

	/* open and load snd card */
	ret = snd_ctl_open(&handle, card_name, SND_CTL_READONLY);
	if (ret < 0) {
		printf("%s: control %s open retor: %s\n", __func__, card_name,
			snd_strerror(ret));
		return ret;
	}

	/* get sound card info */
	ret = snd_ctl_card_info(handle, info);
	if (ret < 0) {
		printf("%s :control %s local retor: %s\n", __func__,
			card_name, snd_strerror(ret));
		goto close;
	}

	/* get the mixer element list */
	ret = snd_ctl_elem_list(handle, list);
	if (ret < 0) {
		printf("%s: cannot determine controls: %s\n", __func__,
			snd_strerror(ret));
		goto close;
	}

	/* get number of elements */
	count = snd_ctl_elem_list_get_count(list);
	if (count < 0) {
		ret = 0;
		goto close;
	}

	/* alloc space for elements */
	snd_ctl_elem_list_set_offset(list, 0);
	if (snd_ctl_elem_list_alloc_space(list, count) < 0) {
		printf("%s: not enough memory...\n", __func__);
		ret =  -ENOMEM;
		goto close;
	}

	/* get list of elements */
	if ((ret = snd_ctl_elem_list(handle, list)) < 0) {
		printf("%s: cannot determine controls: %s\n", __func__,
			snd_strerror(ret));
		goto free;
	}

	/* iterate through list for each kcontrol and display */
	for (i = 0; i < count; ++i) {
		snd_ctl_elem_id_t *id;
		snd_ctl_elem_id_alloca(&id);
		snd_ctl_elem_list_get_id(list, i, id);

		ret = dump_control(handle, id);
		if (ret < 0) {
			printf("%s: cannot determine controls: %s\n",
				__func__, snd_strerror(ret));
			goto free;
		}
	}
free:
	snd_ctl_elem_list_free_space(list);
close:
	snd_ctl_close(handle);
	return ret;
}

/* read PCM data from input file  - does not delete WAV header */
static void *reader_thread(void *data)
{
	struct alsa_source *alsa = (struct alsa_source*)data;
	char * buf = alsa->buffer + alsa->reader_position;
	int bytes = 0, overlap, val, err;
	struct timespec ts;

	while (bytes >= 0) {

        	clock_gettime(CLOCK_REALTIME, &ts);
           	ts.tv_sec += 1; /* wait 1 second for free period */

		/* wait for reader to give us some space */
		err = sem_timedwait(&alsa->sem, &ts);
		if (err < 0) {
			if (pcm_pause)
				continue;
			if (complete)
				break;
			printf("reader timeout\n");
			break;
		}

		/* pointer wrap ? */
		if (alsa->reader_position == alsa->period_bytes * alsa->num_periods) {

			alsa->reader_position = 0;
			buf = alsa->buffer;
		}

		/* write whole period */
		bytes = read(alsa->in_fd, buf, alsa->period_bytes);
		if (bytes < 0)
			break;
		rperiods++;

		/* inc writer pos */
		alsa->reader_position += bytes;

		buf = alsa->buffer + alsa->reader_position;
	}

out:
	complete = 1;
	printf("reader thread finished %d periods\n", rperiods);
	return NULL;
}

/* sets the audio parameters */
static int alsa_set_params(struct alsa_source *alsa_data)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	int err;
	
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	
	/* number of bits per sample e.g. 16 for S16_LE */
	alsa_data->bits_per_sample =
			snd_pcm_format_physical_width(alsa_data->format);

	/* number of bits in alsa frame e.g. 32 for S16_LE stereo */
	alsa_data->bits_per_frame =
		alsa_data->bits_per_sample * alsa_data->channels;

	/* number of frames in a period (period bytes / frame bytes) */
	alsa_data->period_frames = 
		alsa_data->period_bytes / (alsa_data->bits_per_frame >> 3);

	/* config Hardware params */
	err = snd_pcm_hw_params_any(alsa_data->handle, params);
	if (err < 0) {
		printf("Broken configuration for this PCM\n");
		return -ENODEV;
	}
	
	/* set pcm format to be interleaved (e.g. LRLRLR for stereo) */
	err = snd_pcm_hw_params_set_access(alsa_data->handle, params,
						SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		printf("Access type not available\n");
		return -EINVAL;
	}
	
	/* sample format e.g. 16 bit little endian S16_LE */
	err = snd_pcm_hw_params_set_format(alsa_data->handle, params,
						alsa_data->format);
	if (err < 0) {
		printf("Sample format non available\n");
		return -EINVAL;
	}
	
	/* number of channels */
	err = snd_pcm_hw_params_set_channels(alsa_data->handle, params,
						alsa_data->channels);
	if (err < 0) {
		printf("Channels count non available\n");
		return -EINVAL;
	}

	/* rate (or nearest) */
	err = snd_pcm_hw_params_set_rate_near(alsa_data->handle, params,
						&alsa_data->rate, 0);
	if (err < 0) {
		printf("rate non available\n");
		return -EINVAL;
	}

	/* config hardware buffering */

	/* get max supported buffer size */
	err = snd_pcm_hw_params_get_buffer_size_max(params, 
						    &alsa_data->buffer_frames);

	/* we want buffer to be atleast alsa_data->num_dperiods in size */
	if ((alsa_data->buffer_frames / alsa_data->period_frames) < alsa_data->num_dperiods) {
		printf("ALSA buffer too small %ld frames need %ld\n",
		       alsa_data->buffer_frames, 
		       alsa_data->period_frames * (alsa_data->num_dperiods + 1));
		return -EINVAL;
	}

	/* set required period size */
	err = snd_pcm_hw_params_set_period_size(alsa_data->handle, params,
						alsa_data->period_frames, 0);
	if (err < 0) {
		printf("period size not available\n");
		return -EINVAL;
	}

	/* set required buffer size (or nearest)*/
	err = snd_pcm_hw_params_set_buffer_size_near(alsa_data->handle, params,
							     &alsa_data->buffer_frames);
	if (err < 0) {
		printf("buffer size not available\n");
		return -EINVAL;
	}
	
	/* commit all above hardware audio parameters to driver */
	err = snd_pcm_hw_params(alsa_data->handle, params);
	if (err < 0) {
		printf("Unable to install hw params\n");
		return -EINVAL;
	}

	/* config min avail frames to consider PCM ready */
	snd_pcm_sw_params_current(alsa_data->handle, swparams);

	err = snd_pcm_sw_params_set_avail_min(alsa_data->handle, swparams,
						alsa_data->period_frames);
	if (err < 0) {
		printf("failed to set avail min\n");
		return -EINVAL;
	}
	
	/* frames for alsa-lib/driver to buffer internally before starting */ 
	err = snd_pcm_sw_params_set_start_threshold(alsa_data->handle,
							swparams, 1);
	if (err < 0) {
		printf("failed to set start threshold\n");
		return -EINVAL;
	}
	
	/* if free frames >= buffer frames then stop */
	err = snd_pcm_sw_params_set_stop_threshold(alsa_data->handle, swparams,
							alsa_data->buffer_frames);
	if (err < 0) {
		printf("failed to set stop threshold\n");
		return -EINVAL;
	}

	/* commit the software params to alsa-lib */
	if (snd_pcm_sw_params(alsa_data->handle, swparams) < 0) {
		printf("unable to install sw params\n");
		snd_pcm_sw_params_dump(swparams, log);
		return -EINVAL;
	}

	snd_pcm_dump(alsa_data->handle, log);
	return 0;
}

void *pcm_pause_resume_thread(void *data) 
{
	struct alsa_source *alsa = (struct alsa_source*)data;
	char ch;

	/* Get the input in a loop */
	while (!complete) {
		scanf("%c",&ch);
		if (ch == 'p' || ch == 'P') {
			pcm_pause = 1;
		}
		if (ch == 'r' || ch == 'R') {
			sem_post(&alsa->rsem);
			pcm_pause = 0;
		}
	}
	return NULL;
}

/* write pcm data from the audio driver */
static ssize_t alsa_pcm_write(struct alsa_source *alsa, size_t count)
{
	char *data = alsa->buffer + alsa->writer_position, ch;
	ssize_t size, result = 0;
	snd_pcm_uframes_t frames;
	int wait, val;

	/* is there data for playback */
	sem_getvalue(&alsa->sem, &val);

	/* has the writer been starved by a slow reader ??? */
	if (val == alsa->num_periods)
		printf("w: starved %d\n", val);
	if (val < alsa->num_periods)
		alsa->debug_avail_per[(alsa->num_periods - 1) - val]++;

	/* change count from bytes to frames */
	frames = count / (alsa->bits_per_frame >> 3);

	/* read a number of frames from the driver */
	while (frames > 0) {
		size = snd_pcm_writei(alsa->handle, data, frames);
		
		if (size == -EAGAIN || (size >= 0 && (size_t)size < frames)) {
			printf("write failed\n");
		} else if (size == -EPIPE) {
			printf("xrun();\n");
			snd_pcm_recover(alsa->handle, size, 0);
		} else if (size == -ESTRPIPE) {
			printf("suspend();\n");
		} else if (size < 0) {
			printf("write error: %s", snd_strerror(size));
		}
		
		/* still have data to write ? */
		if (size > 0) {
			result += size;
			frames -= size;
			data += size * alsa->bits_per_frame / 8;
		}

		/* do we need to pause or resume */
		if (pcm_pause > 0) {
			fprintf(stdout,"\rpaused\n");
			snd_pcm_pause(alsa->handle, 1);
			sem_wait(&alsa->rsem);
			fprintf(stdout,"\rresumed\n");
			snd_pcm_pause(alsa->handle, 0);
		}
	}
	
	/* got frames, so update reader pointer */
	alsa->writer_position += count;

	/* buffer wrap ? */
	if (alsa->writer_position >= alsa->period_bytes * alsa->num_periods)
		alsa->writer_position = 0;

	/* let reader get another period */
	sem_post(&alsa->sem);
	
	wperiods++;

	return result;
}

/* CTRL - C handler - dump used period debug values */
static void shutdown(int sig)
{
	if (irq == 1)
		exit(0);

	complete = 1;
	irq = 1;
}

static const struct option long_options[] = {
	{"rate", 1, 0, 'r'},
	{"fmt", 1, 0, 'f'},
	{"channels", 1, 0, 'c'},
	{"periods", 1, 0, 'p'},	/* numner of user periods */
	{"size", 1, 0, 's'},	/* period size */
	{"dperiods", 1, 0, 'd'},	/* number of driver periods */
}; 

static const char short_options[] = "r:f:c:p:s:d:";

int main(int argc, char *argv[])
{
	struct alsa_source *alsa_data;
	int ret, not_ready = 1, val, option = 0, i, time;
	char *in_file, c;

	alsa_data = calloc(1, sizeof(struct alsa_source));
	if (alsa_data == NULL)
		return -ENOMEM;

	/* default - configure via cmd line */
	alsa_data->channels = 2;
	alsa_data->rate = 44100;
	alsa_data->format = SND_PCM_FORMAT_S16_LE;
	alsa_data->period_bytes = 8820 * alsa_data->channels;
	alsa_data->num_periods = 32;
	alsa_data->num_dperiods = 3;

	while ((c = getopt_long(argc, argv, short_options, long_options, &option)) != -1) {
		switch (c) {
		case 'c':
			alsa_data->channels = strtol(optarg, NULL, 0);
			break;
		case 'f':
			alsa_data->format = snd_pcm_format_value(optarg);
			if (alsa_data->format == SND_PCM_FORMAT_UNKNOWN) {
				printf("wrong extended format %s\n", optarg);
				exit(-EINVAL);
			}
			break;
		case 'r':
			alsa_data->rate = strtol(optarg, NULL, 0);
			break;
		case 'p':
			alsa_data->num_periods = strtol(optarg, NULL, 0);
			break;
		case 'd':
			alsa_data->num_dperiods = strtol(optarg, NULL, 0);
			break;
		case 's':
			alsa_data->period_bytes = strtol(optarg, NULL, 0);
			break;
		default:
			printf("%s invalid option\n", argv[0]);
			return 1;
		}
	}

	printf("%s playing %d chans using %d bytes\n", argv[0], alsa_data->channels,
		alsa_data->period_bytes * alsa_data->num_periods * alsa_data->channels);
	
	alsa_data->buffer = malloc(alsa_data->period_bytes *
		alsa_data->num_periods * alsa_data->channels);
	if (alsa_data->buffer == NULL)
		return -ENOMEM;
	alsa_data->debug_avail_per = calloc(alsa_data->num_periods, sizeof(int));
	if (alsa_data->debug_avail_per == NULL)
		return -ENOMEM;

	if (argc > option) {
		in_file = argv[optind];
	} else {
		printf(" no playback file specified\n");
		exit(-EINVAL);
	}
		
	/* for debug */
	ret = snd_output_stdio_attach(&log, stderr, 0);

	/* display controls */
	mixers_display(log, "default");
	
	/* catch ctrl C and dump buffer info */
	signal(SIGINT, shutdown);

	/* the input file */
	alsa_data->in_fd = open(in_file, O_RDWR);
	if (alsa_data->in_fd < 0) {
		printf("failed to open %s err %d\n",
			in_file, alsa_data->in_fd);
		return alsa_data->in_fd;
	}

	/* open the alsa source */
	ret = snd_pcm_open(&alsa_data->handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (ret < 0) {
		printf("audio open error: %s\n", snd_strerror(ret));
		return ret;
	}
	
	/* configure audio */
	ret = alsa_set_params(alsa_data);
	if (ret < 0) {
		printf("set params error: %d\n", ret);
		return ret;
	}
	
	/* init threads */
	pthread_mutex_init(&alsa_data->mutex, NULL);
	sem_init(&alsa_data->sem, 0, alsa_data->num_periods);
	sem_init(&alsa_data->rsem, 0, 0);
	ret = pthread_create(&alsa_data->tid, NULL, reader_thread, alsa_data);
	if (ret < 0) {
		printf("failed to create thread %d\n", ret);
		return ret;
	}
	ret = pthread_create(&alsa_data->pid, NULL, pcm_pause_resume_thread, alsa_data);
	if (ret < 0) {
		printf("failed to create thread %d\n", ret);
		return ret;
	}

	/* buffer at least half the periods before staring playback */
	while (not_ready) {
		sem_getvalue(&alsa_data->sem, &val);
		if (val < (alsa_data->num_periods / 2)) {
			not_ready = 0;
			break;
		}
		usleep(1000 * 50); /* wait 50 ms */
	}

	/* todo - exit the loop after n iterations */
	while (!complete) {
		alsa_pcm_write(alsa_data, alsa_data->period_bytes * alsa_data->channels);
	}

	/* this may be different from writer periods depending where in
	   the read/write cycle ctrl-C was pressed */
	printf("writer finished %d periods\n", wperiods);

	/* wait for reader to finish */
	pthread_join(alsa_data->tid, NULL);

	if (irq) {
		printf("PCM buffers waiting to be read from file (high means file IO is slow)\n");

		/* dump debug */
		for (i = 0; i < alsa_data->num_periods; i++) {
			if (alsa_data->debug_avail_per[i])
				printf("Empty Buffers %d (from %d) - occurence %3.3f%% - %d\n",
					alsa_data->num_periods - i, alsa_data->num_periods,
					((double)alsa_data->debug_avail_per[i] / wperiods) * 100,
					alsa_data->debug_avail_per[i]);
		}
	}

	/* cleanup */
	printf("draining buffers ...\n");
	snd_pcm_nonblock(alsa_data->handle, 0);
	snd_pcm_drain(alsa_data->handle);
	snd_pcm_close(alsa_data->handle);
	snd_output_close(log);
	return 0;
}


