/*
* OMAP4 ALSA/TinyAlsa Voice Call control
*
*  Copyright (C) 2010-2011 Texas Instruments, Inc.
*
*  Author : Francois Mazard (f-mazard@ti.com)
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*
*/

/*#define VERSION "0.1" initial version */
/*#define VERSION "0.2" Update to support TinyAlsa */
#define VERSION "0.3" /* Update to ICS Nexus Prime */

#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <getopt.h>

#ifdef TINYALSA
    #include <tinyalsa/asoundlib.h>
#else
    #include <alsa/asoundlib.h>
#endif

#ifdef HOSTMODE
#ifdef TINYALSA
    #ERROR "Hostmode is not compatible with TinyAlsa !!!"
#endif
#endif

/* -----------------------------------------------------*/
enum audioMode {
	 HANDSET = 0,
	 HEADSET,
	 HANDFREE,
	 UNKNOWN
};

struct audioConfig {
	const char *name;
	const char *value;
};
/* -----------------------------------------------------*/
#ifdef TINYALSA
#define snd_strerror(error)  strerror(error)
#endif

#define DEBUG(arg...) \
	if (debugFlag) \
		fprintf(stdout, arg)

#define SND_ERROR_EXIT(function) \
	if ((error = function) < 0) {\
		fprintf(stderr, "error in function '%s' line %d: %s(%d)\n", __FUNCTION__, __LINE__, snd_strerror(error), error); \
		exit(EXIT_FAILURE); \
	}

#define SND_ERROR_RETURN(function) \
	if ((error = function) < 0) {\
		fprintf(stderr, "error in function '%s' line %d: %s(%d)\n", __FUNCTION__, __LINE__, snd_strerror(error), error); \
		return error; \
	}

#ifndef TINYALSA
#define COMMAND_SIZE 256
#define AUDIO_MODEM_PCM_LATENCY     500000
#endif
/* -----------------------------------------------------*/
int debugFlag = 0, terminate = 0, sampleRate = 8000, channels = 2, sideTone=1, volumeChanged = 0;
enum audioMode mode = UNKNOWN;
#ifdef TINYALSA
unsigned int pcmName = 5;
struct mixer *mixer;
struct pcm_config pcmConfig = {
    .period_size = 160,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};
struct pcm *pcmdl, *pcmul;
#else
snd_pcm_t *playbackHandle,*captureHandle;
char *pcmName = "hw:0,5";
char *alsaControlCommandName = "alsa_amixer";
#endif
char *powerLevel = "Low";
char *volume = "110";
char *distro = "ICS";
/* -----------------------------------------------------*/

const struct audioConfig handsetAudioConfig_GB[] = {
	/* Downlink */
		/* TWL6040 */
		{"HS Left Playback",			"off"},
		{"HS Right Playback",			"off"},
		{"Headset Playback Volume",		"0"},
		{"HF Left Playback",			"off"},
		{"HF Right Playback",			"off"},
		{"Handsfree Playback Volume",		"0"},
		{"Earphone Driver Switch",		"on"},
		{"Earphone Playback Volume",		"13"},
		/* ABE */
		{"DL1 PDM Switch",			"on"},
		{"Sidetone Mixer Playback",		"on"},
		{"SDT DL Volume",			"120"},
		{"DL1 Mixer Voice",			"on"},
		{"DL1 Voice Playback Volume",		"110"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Main Mic"},
		{"Analog Right Capture Route",		"Sub Mic"},
		{"Capture Preamplifier Volume",		"1"},
		{"Capture Volume",			"4"},
		/* ABE */
		{"AMIC_UL PDM Switch",			"on"},
		{"MUX_VX0",				"AMic0"},
		{"MUX_VX1",				"AMic1"},

		{"AUDUL Voice UL Volume",		"120"},
		{"Voice Capture Mixer Capture",		"on"},
	/* Sidetone */
		{"Sidetone Mixer Capture",		"on"},
		{"SDT UL Volume",			"90"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig handfreeAudioConfig_GB[] = {
	/* Downlink */
		/* TWL6040 */
		{"Earphone Driver Switch",		"off"},
		{"Earphone Playback Volume",		"0"},
		{"HS Left Playback",			"off"},
		{"HS Right Playback",			"off"},
		{"Headset Playback Volume",		"0"},
		{"HF Left Playback",			"HF DAC"},
		{"HF Right Playback",			"HF DAC"},
		{"Handsfree Playback Volume",		"23"},
		/* ABE */
		{"DL1 PDM Switch",			"on"},
		{"DL2 Left Equalizer",			"0"},
		{"DL2 Right Equalizer",			"0"},
		{"DL2 Mixer Voice",			"on"},
		{"DL2 Voice Playback Volume",		"110"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Main Mic"},
		{"Analog Right Capture Route",		"Sub Mic"},
		{"Capture Preamplifier Volume",		"1"},
		{"Capture Volume",			"4"},
		/* ABE */
		{"AMIC_UL PDM Switch",			"on"},
		{"MUX_VX0",				"AMic0"},
		{"MUX_VX1",				"AMic1"},

		{"AUDUL Voice UL Volume",		"120"},
		{"Voice Capture Mixer Capture",		"on"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig headsetAudioConfig_GB[] = {
	/* Downlink */
		/* TWL6040 */
		{"Earphone Driver Switch",		"off"},
		{"Earphone Playback Volume",		"0"},
		{"HF Left Playback",			"off"},
		{"HF Right Playback",			"off"},
		{"Handsfree Playback Volume",		"0"},
		{"HS Left Playback",			"HS DAC"},
		{"HS Right Playback",			"HS DAC"},
		{"Headset Playback Volume",		"13"},
		/* ABE */
		{"DL1 PDM Switch",			"on"},
		{"Sidetone Mixer Playback",		"on"},
		{"SDT DL Volume",			"120"},
		{"DL1 Mixer Voice",			"on"},
		{"DL1 Voice Playback Volume",		"110"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Headset Mic"},
		{"Analog Right Capture Route",		"Headset Mic"},
		{"Capture Preamplifier Volume",		"1"},
		{"Capture Volume",			"4"},
		/* ABE */
		{"AMIC_UL PDM Switch",			"on"},
		{"MUX_VX0",				"AMic0"},
		{"MUX_VX1",				"AMic1"},

		{"AUDUL Voice UL Volume",		"120"},
		{"Voice Capture Mixer Capture",		"on"},
	/* Sidetone */
		{"Sidetone Mixer Capture",		"on"},
		{"SDT UL Volume",			"90"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig offAudioConfig_GB[] = {
	/* Downlink */
		/* TWL6040 */
		{"Earphone Driver Switch",		"off"},
		{"Earphone Playback Volume",		"0"},
		{"HS Left Playback",			"off"},
		{"HS Right Playback",			"off"},
		{"Headset Playback Volume",		"0"},
		{"HF Left Playback",			"off"},
		{"HF Right Playback",			"off"},
		{"Handsfree Playback Volume",		"0"},
		/* ABE */
		{"DL1 PDM Switch",			"off"},
		{"Sidetone Mixer Playback",		"off"},
		{"SDT DL Volume",			"0"},
		{"DL1 Mixer Voice",			"off"},
		{"DL1 Voice Playback Volume",		"0"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"off"},
		{"Analog Right Capture Route",		"off"},
		{"Capture Preamplifier Volume",		"off"},
		{"Capture Volume",			"off"},
		/* ABE */
		{"AMIC_UL PDM Switch",			"off"},
		{"MUX_VX0",				"None"},
		{"MUX_VX1",				"None"},

		{"AUDUL Voice UL Volume",		"0"},
		{"Voice Capture Mixer Capture",		"off"},
	/* Sidetone */
		{"Sidetone Mixer Capture",		"off"},
		{"SDT UL Volume",			"0"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig handsetAudioConfig_ICS[] = {
	/* Downlink */
		/* TWL6040 */
		{"HS Left Playback",			"Off"},
		{"HS Right Playback",			"Off"},
		{"Headset Playback Volume",		"0"},
		{"HF Left Playback",			"Off"},
		{"HF Right Playback",			"Off"},
		{"Handsfree Playback Volume",		"0"},
		{"Earphone Enable Switch",		"On"},
		{"Earphone Playback Volume",		"13"},
		/* ABE */
		{"DL1 PDM Switch",			"On"},
		{"DL1 Equalizer",			"4Khz LPF   0dB"},
		{"Sidetone Mixer Playback",		"On"},
		{"SDT DL Volume",			"120"},
		{"DL1 Mixer Voice",			"On"},
		{"DL1 Voice Playback Volume",		"110"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Main Mic"},
		{"Analog Right Capture Route",		"Sub Mic"},
		{"Capture Preamplifier Volume",		"1"},
		{"Capture Volume",			"4"},
		/* ABE */
		{"MUX_VX0",				"AMic0"},
		{"MUX_VX1",				"AMic1"},
		{"AMIC UL Volume",			"120"},
		{"Voice Capture Mixer Capture",		"On"},
	/* Sidetone */
		{"Sidetone Mixer Capture",		"On"},
		{"SDT UL Volume",			"90"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig handfreeAudioConfig_ICS[] = {
	/* Downlink */
		/* TWL6040 */
		{"Earphone Enable Switch",		"Off"},
		{"Earphone Playback Volume",		"0"},
		{"HS Left Playback",			"Off"},
		{"HS Right Playback",			"Off"},
		{"Headset Playback Volume",		"0"},
		{"HF Left Playback",			"HF DAC"},
		{"HF Right Playback",			"HF DAC"},
		{"Handsfree Playback Volume",		"23"},
		/* ABE */
		{"DL2 Left Equalizer",			"High-pass 0dB"},
		{"DL2 Right Equalizer",			"High-pass 0dB"},
		{"DL2 Mixer Voice",			"On"},
		{"DL2 Voice Playback Volume",		"110"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Main Mic"},
		{"Analog Right Capture Route",		"Sub Mic"},
		{"Capture Preamplifier Volume",		"1"},
		{"Capture Volume",			"4"},
		/* ABE */
		{"MUX_VX0",				"AMic0"},
		{"MUX_VX1",				"AMic1"},
		{"AUDUL Voice UL Volume",		"120"},
		{"Voice Capture Mixer Capture",		"On"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig headsetAudioConfig_ICS[] = {
	/* Downlink */
		/* TWL6040 */
		{"Earphone Enable Switch",		"Off"},
		{"Earphone Playback Volume",		"0"},
		{"HF Left Playback",			"Off"},
		{"HF Right Playback",			"Off"},
		{"Handsfree Playback Volume",		"0"},
		{"HS Left Playback",			"HS DAC"},
		{"HS Right Playback",			"HS DAC"},
		{"Headset Playback Volume",		"13"},
		/* ABE */
		{"DL1 PDM Switch",			"On"},
		{"DL1 Equalizer",			"4Khz LPF   0dB"},
		{"Sidetone Mixer Playback",		"On"},
		{"SDT DL Volume",			"120"},
		{"DL1 Mixer Voice",			"On"},
		{"DL1 Voice Playback Volume",		"110"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Headset Mic"},
		{"Analog Right Capture Route",		"Headset Mic"},
		{"Capture Preamplifier Volume",		"1"},
		{"Capture Volume",			"4"},
		/* ABE */
		{"MUX_VX0",				"AMic0"},
		{"MUX_VX1",				"AMic1"},
		{"AUDUL Voice UL Volume",		"120"},
		{"Voice Capture Mixer Capture",		"On"},
	/* Sidetone */
		{"Sidetone Mixer Capture",		"On"},
		{"SDT UL Volume",			"90"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig offAudioConfig_ICS[] = {
	/* Downlink */
		/* TWL6040 */
		{"Earphone Enable Switch",		"Off"},
		{"Earphone Playback Volume",		"0"},
		{"HS Left Playback",			"Off"},
		{"HS Right Playback",			"Off"},
		{"Headset Playback Volume",		"0"},
		{"HF Left Playback",			"Off"},
		{"HF Right Playback",			"Off"},
		{"Handsfree Playback Volume",		"0"},
		/* ABE */
		{"DL1 PDM Switch",			"Off"},
		{"Sidetone Mixer Playback",		"Off"},
		{"SDT DL Volume",			"0"},
		{"DL1 Mixer Voice",			"Off"},
		{"DL1 Voice Playback Volume",		"0"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Off"},
		{"Analog Right Capture Route",		"Off"},
		{"Capture Preamplifier Volume",		"Off"},
		{"Capture Volume",			"Off"},
		/* ABE */
		{"MUX_VX0",				"None"},
		{"MUX_VX1",				"None"},
		{"AUDUL Voice UL Volume",		"0"},
		{"Voice Capture Mixer Capture",		"Off"},
	/* Sidetone */
		{"Sidetone Mixer Capture",		"Off"},
		{"SDT UL Volume",			"0"},
	/* eof */
		{ "eof", ""},
};
/* -----------------------------------------------------*/
static void show_help(void)
{
	puts("Usage: voicecallcontrol [options]");
	puts("Useful options:\n"
		   "  -h, --help			this help\n"
		   "  -v, --version			version\n"
		   "  -m, --mode=NAME		audio mode: handset, handfree, headset\n"
		   "  -V, --volume=#		volume (min=0,step=1,max=120, default=110)\n"
		   "  -r, --rate=#			sample rate in Hz (default 8000)\n"
		   "  -D, --device=NAME		select PCM by name (default hw:0,5 for ALSA, 5 for TinyAlsa)\n"
		   "  -c, --channels=#		channels (default 2)\n"
		   "  -p, --power=NAME		select power level (Low or High, default=Low)\n"
		   "  -i, --distro=DISTRIBUTION	select distro supported (GB, ICS, default=ICS)\n"
#ifdef TINYALSA
		   "  -s, --sidetone=#		enable(1)/disable(0) sidetone (default=1)");
#else
		   "  -s, --sidetone=#		enable(1)/disable(0) sidetone (default=1)\n"
		   "  -C, --command=NAME		select amixer command name");

#endif
	puts("Debugging options:\n"
		   "  -d, --debug			toggle debugging trace\n");
}

static void show_version(void)
{
	fprintf(stdout, "Version: %s\n", VERSION);
}

void sigHandler(int sig) {
	DEBUG("voiceCallControl %s: terminate sig(%d)\n", __FUNCTION__, sig);
	terminate = 1;
}

void initSig(void)
{
	struct sigaction sigAction;

	memset(&sigAction, 0, sizeof(sigAction));

	sigAction.sa_flags   = SA_NOCLDSTOP;
	sigAction.sa_handler = sigHandler;
	sigaction(SIGTERM, &sigAction, NULL);
	sigaction(SIGINT,  &sigAction, NULL);
	sigaction(SIGPIPE, &sigAction, NULL);
	sigaction(SIGKILL, &sigAction, NULL);
}

static void parse_options(int argc, char *argv[])
{
#ifdef TINYALSA
	static const char short_options[] = "hvm:V:r:D:c:p:s:d";
#else
	static const char short_options[] = "hvm:V:r:D:c:p:i:s:C:d";
#endif
	static const struct option long_options[] = {
		{ .name = "help", .val = 'h' },
		{ .name = "version", .val = 'v' },
		{ .name = "mode", .has_arg = 1, .val = 'm' },
		{ .name = "volume", .has_arg = 1, .val = 'V' },
		{ .name = "rate", .has_arg = 1, .val = 'r' },
		{ .name = "device", .has_arg = 1, .val = 'D' },
		{ .name = "channels", .has_arg = 1, .val = 'c' },
		{ .name = "power", .has_arg = 1, .val = 'p' },
		{ .name = "distro", .has_arg = 1, .val = 'i' },
		{ .name = "sidetone", .has_arg = 1, .val = 's' },
#ifndef TINYALSA
		{ .name = "command", .has_arg = 1, .val = 'C' },
#endif
		{ .name = "debug", .val = 'd' },
		{ .name = ""}
	};
	int option;

	while ((option = getopt_long(argc, argv, short_options,
				     long_options, NULL)) != -1) {
		switch (option) {
		case '?':
		case 'h':
			show_help();
			exit(EXIT_SUCCESS);
		case 'v':
			show_version();
			exit(EXIT_SUCCESS);
		case 'd':
			debugFlag = 1;
			break;
		case 'm':
			if (!strcmp(optarg, "handset"))
				mode = HANDSET;
			else if (!strcmp(optarg, "handfree"))
				mode = HANDFREE;
			else if (!strcmp(optarg, "headset"))
				mode = HEADSET;
			else {
				fprintf(stderr, "unknown audio mode: %s\n", optarg);
				goto fail;
			}
			break;
		case 'V':
			volumeChanged = 1;
			volume = optarg;
			break;
		case 'r':
			sampleRate = strtol(optarg, NULL, 0);
			break;
		case 'c':
			channels = strtol(optarg, NULL, 0);
			break;
		case 'D':
#ifdef TINYALSA
			pcmName = strtol(optarg, NULL, 0);
#else
			pcmName = optarg;
#endif
			break;
		case 'p':
			powerLevel = optarg;
			break;
		case 'i':
			if ((!strcmp(optarg, "GB")) ||
			    (!strcmp(optarg, "ICS")))
				distro = optarg;
			else {
				fprintf(stderr, "unknown distro: %s\n", optarg);
				goto fail;
			}
			break;
		case 's':
			sideTone = strtol(optarg, NULL, 0);
			break;
#ifndef TINYALSA
		case 'C':
			alsaControlCommandName = optarg;
			break;
#endif
		default:
			fprintf(stderr, "unknown option: %c\n", option);
fail:
			fputs("try `voicecallcontrol --help' for more information\n", stderr);
			exit(EXIT_FAILURE);
		}
	}
}

int setControlName(const char *name, const char *value)
{
	int error = 0;
#ifdef TINYALSA
	struct mixer_ctl *ctl;
	enum mixer_ctl_type type;
	unsigned int num_values, i;
	int digit_value;

	ctl = mixer_get_ctl_by_name(mixer, name);
	type = mixer_ctl_get_type(ctl);
	num_values = mixer_ctl_get_num_values(ctl);

	DEBUG("%s: name: %s, value: %s, type: %s, num_values: %d\n", __FUNCTION__,
								     name,
								     value,
								     mixer_ctl_get_type_string(ctl),
								     num_values);

	switch (type)
	{
		case MIXER_CTL_TYPE_INT:
			digit_value = atoi(value);
			for (i = 0; i < num_values; i++) {
				SND_ERROR_RETURN(mixer_ctl_set_value(ctl, i, digit_value));
			}
			break;
		case MIXER_CTL_TYPE_BOOL:
			if ((!strcmp(value, "on")) || (!strcmp(value, "On"))) {
				SND_ERROR_RETURN(mixer_ctl_set_value(ctl, i, 1));
			} else if ((!strcmp(value, "off")) || (!strcmp(value, "Off"))) {
				SND_ERROR_RETURN(mixer_ctl_set_value(ctl, i, 0));
			} else {
				fputs("Wrong boolean value\n", stderr);
				error = -EINVAL;
			}
			break;
		case MIXER_CTL_TYPE_ENUM:
			SND_ERROR_RETURN(mixer_ctl_set_enum_by_string(ctl, value));
			break;
		default:
			fputs("Wrong type!!!\n", stderr);
			error = -EINVAL;
			break;
	}
#else
	char command[COMMAND_SIZE];

	if (debugFlag)
		sprintf(command, "%s -d cset name=\"%s\" \"%s\"", alsaControlCommandName, name, value);
	else
		sprintf(command, "%s -q cset name=\"%s\" \"%s\"", alsaControlCommandName, name, value);

	DEBUG("%s: system call: '%s'\n", __FUNCTION__, &command[0]);
#ifndef HOSTMODE
	error = system(&command[0]);
#endif
#endif
	return error;
}

int setAudioConfig(const struct audioConfig *audiocfg)
{
	int error = 0;
	struct audioConfig *config = (struct audioConfig *)audiocfg;

	/* Set the power level */
	DEBUG("%s: Power Level = %s\n", __FUNCTION__, powerLevel);
	if (!strcmp(powerLevel, "High")) {
		error = setControlName("TWL6040 Power Mode", "High-Performance");
	} else {
		error = setControlName("TWL6040 Power Mode", "Low-Power");
	}
	while(strcmp(config->name, "eof") && !error) {
		if (!sideTone) {
			if (!strcmp(config->name, "Sidetone Mixer Capture")) {
				DEBUG("%s: %s = off\n", __FUNCTION__, config->name);
				error = setControlName(config->name, "Off");
				config++;
				continue;
			}
		}

		if (volumeChanged) {
			if (!strcmp(config->name, "DL1 Voice Playback Volume") ||
				!strcmp(config->name, "DL2 Voice Playback Volume")) {
				DEBUG("%s: %s = %s\n", __FUNCTION__, config->name, volume);
				error = setControlName(config->name, volume);
				config++;
				continue;
			}
		}

		DEBUG("%s: %s = %s\n", __FUNCTION__, config->name, config->value);
		error = setControlName(config->name, config->value);
		config++;
	}

	return error;
}
#ifndef TINYALSA
static void ALSAErrorHandler(const char *file,
			     int line,
			     const char *function,
			     int err,
			     const char *fmt,
			     ...)
{
	char buf[BUFSIZ];
	va_list arg;
	int l;

	va_start(arg, fmt);
	l = snprintf(buf, BUFSIZ, "%s:%i:(%s) ", file, line, function);
	vsnprintf(buf + l, BUFSIZ - l, fmt, arg);
	buf[BUFSIZ-1] = '\0';
	fprintf(stderr, "Error ALSALib: %s", buf);
	va_end(arg);
}
#endif
int startPcm(void)
{
#ifdef TINYALSA

	DEBUG("%s:  PCM name: %d sampleRate: %d channels: %d\n", __FUNCTION__, pcmName, sampleRate, channels);

	pcmConfig.channels = channels;
	pcmConfig.rate = sampleRate;

	pcmdl = pcm_open(0, pcmName , PCM_OUT, &pcmConfig);
	if (!pcm_is_ready(pcmdl)) {
	    fprintf(stderr, "cannot open PCM modem DL stream: %s", pcm_get_error(pcmdl));
	    goto err_open_dl;
	}

	pcmul = pcm_open(0, pcmName , PCM_IN, &pcmConfig);
	if (!pcm_is_ready(pcmul)) {
	    fprintf(stderr, "cannot open PCM modem UL stream: %s", pcm_get_error(pcmdl));
	    goto err_open_ul;
	}

	pcm_start(pcmdl);
	pcm_start(pcmul);

	return 0;

err_open_ul:
	pcm_close(pcmul);
err_open_dl:
	pcm_close(pcmdl);

	return -ENOMEM;
#else
	int error = 0;

	DEBUG("%s:  PCM name: %s sampleRate: %d channels: %d\n", __FUNCTION__, pcmName, sampleRate, channels);
#ifndef HOSTMODE
	SND_ERROR_RETURN(snd_pcm_open(&captureHandle, pcmName, SND_PCM_STREAM_CAPTURE, 0));
	SND_ERROR_RETURN(snd_pcm_open(&playbackHandle, pcmName, SND_PCM_STREAM_PLAYBACK, 0));

	SND_ERROR_RETURN(snd_pcm_set_params(captureHandle,
										SND_PCM_FORMAT_S16_LE,
										SND_PCM_ACCESS_RW_INTERLEAVED,
										channels,
										sampleRate,
										1,
										AUDIO_MODEM_PCM_LATENCY));
	SND_ERROR_RETURN(snd_pcm_set_params(playbackHandle,
									SND_PCM_FORMAT_S16_LE,
									SND_PCM_ACCESS_RW_INTERLEAVED,
									channels,
									sampleRate,
									1,
									AUDIO_MODEM_PCM_LATENCY));

	SND_ERROR_RETURN(snd_pcm_start(captureHandle));
	SND_ERROR_RETURN(snd_pcm_start(playbackHandle));
#endif
	return error;
#endif
}

int stopPcm(void)
{
	int error = 0;
#ifdef TINYALSA
	DEBUG("%s:  PCM name: %d sampleRate: %d channels: %d\n", __FUNCTION__, pcmName, sampleRate, channels);

	SND_ERROR_RETURN(pcm_stop(pcmdl));
	SND_ERROR_RETURN(pcm_stop(pcmul));
	SND_ERROR_RETURN(pcm_close(pcmdl));
	SND_ERROR_RETURN(pcm_close(pcmul));

#else
	DEBUG("%s:  PCM name: %s sampleRate: %d channels: %d\n", __FUNCTION__, pcmName, sampleRate, channels);
#ifndef HOSTMODE
	SND_ERROR_RETURN(snd_pcm_drop(captureHandle));
	SND_ERROR_RETURN(snd_pcm_drop(playbackHandle));

	SND_ERROR_RETURN(snd_pcm_close(captureHandle));
	SND_ERROR_RETURN(snd_pcm_close(playbackHandle));
#endif
#endif
	return error;
}

int main(int argc ,char *argv[])
{
	int error = 0;

#ifdef TINYALSA
	puts("running with TinyAlsa");
#else
#ifdef HOSTMODE
	puts("running in host mode");
#else
	puts("running with Alsa");
#endif
#endif
	if (argc == 1) {
		show_help();
		exit(EXIT_SUCCESS);
	}

	initSig();
	parse_options(argc, argv);

	DEBUG("audio mode chosen: %d\n", mode);
#ifndef TINYALSA
#ifndef HOSTMODE
	SND_ERROR_EXIT(snd_lib_error_set_handler(&ALSAErrorHandler));
#endif
#else
	mixer = mixer_open(0);
	if (NULL == mixer) {
		fprintf(stderr, "problem to open TinyAlsa mixer\n");
		exit(EXIT_FAILURE);
	} else {
		DEBUG("open TinyAlsa mixer\n");
	}
#endif
	switch (mode) {
		case HANDSET:
			if (!strcmp(distro, "GB")) {
				SND_ERROR_EXIT(setAudioConfig(handsetAudioConfig_GB));
			} else if (!strcmp(distro, "ICS")) {
				SND_ERROR_EXIT(setAudioConfig(handsetAudioConfig_ICS));
			}
			break;
		case HANDFREE:
			if (!strcmp(distro, "GB")) {
				SND_ERROR_EXIT(setAudioConfig(handfreeAudioConfig_GB));
			} else if (!strcmp(distro, "ICS")) {
				SND_ERROR_EXIT(setAudioConfig(handfreeAudioConfig_ICS));
			}
			break;
		case HEADSET:
			if (!strcmp(distro, "GB")) {
				SND_ERROR_EXIT(setAudioConfig(headsetAudioConfig_GB));
			} else if (!strcmp(distro, "ICS")) {
				SND_ERROR_EXIT(setAudioConfig(headsetAudioConfig_ICS));
			}
			break;
		default:
			fprintf(stderr, "unknown audio mode: %c\n", mode);
			exit(EXIT_FAILURE);
	}
	SND_ERROR_EXIT(startPcm());

	puts("Audio Voice Call on...\n");
	while(!terminate) {
		sleep(1);
	}

	SND_ERROR_EXIT(stopPcm());
	if (!strcmp(distro, "GB")) {
		SND_ERROR_EXIT(setAudioConfig(offAudioConfig_GB));
	} else if (!strcmp(distro, "ICS")) {
		SND_ERROR_EXIT(setAudioConfig(offAudioConfig_ICS));
	}
	puts("Audio Voice Call off\n");
	exit(EXIT_SUCCESS);
}

