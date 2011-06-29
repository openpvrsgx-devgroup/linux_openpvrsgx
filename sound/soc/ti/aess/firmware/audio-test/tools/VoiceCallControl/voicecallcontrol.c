/*
* OMAP4 ALSA Voice Call control 
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

#define VERSION "0.1" /* initial version */

#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <getopt.h>

#include <alsa/asoundlib.h>
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

#define COMMAND_SIZE 256
#define AUDIO_MODEM_PCM_LATENCY     500000
/* -----------------------------------------------------*/
int debugFlag = 0, terminate = 0, sampleRate = 8000, channels = 1, sideTone=1, volumeChanged = 0;
enum audioMode mode = UNKNOWN;
snd_pcm_t *playbackHandle,*captureHandle;
char *pcmName = "hw:0,5";
char *alsaControlCommandName = "alsa_amixer";
char *powerLevel = "Low";
char *volume = "110";
/* -----------------------------------------------------*/

const struct audioConfig handsetAudioConfig[] = {
	/* Downlink */
		/* TWL6040 */
		{"HS Left Playback",			"off"},
		{"HS Right Playback",			"off"},
		{"Headset Playback Volume",		"0"},
		{"HF Left Playback",			"off"},
		{"HF Right Playback",			"off"},
		{"Handsfree Playback Volume",	"0"},
		{"Earphone Driver Switch",		"on"},
		{"Earphone Playback Volume",		"13"},
		/* ABE */
		{"DL1 PDM Switch",				"on"},
		{"Sidetone Mixer Playback",		"on"},
		{"SDT DL Volume",				"120"},
		{"DL1 Mixer Voice",				"on"},
		{"DL1 Voice Playback Volume",	"110"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Main Mic"},
		{"Analog Right Capture Route",	"Sub Mic"},
		{"Capture Preamplifier Volume",	"1"},
		{"Capture Volume",				"4"},
		/* ABE */
		{"AMIC_UL PDM Switch",			"on"},
		{"MUX_VX0",					"AMic0"},
		{"MUX_VX1",					"AMic1"},
		
		{"AUDUL Voice UL Volume",		"120"},
		{"Voice Capture Mixer Capture",	"on"},
	/* Sidetone */
		{"Sidetone Mixer Capture",		"on"},
		{"SDT UL Volume",				"90"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig handfreeAudioConfig[] = {
	/* Downlink */
		/* TWL6040 */
		{"Earphone Driver Switch",		"off"},
		{"Earphone Playback Volume",		"0"},
		{"HS Left Playback",			"off"},
		{"HS Right Playback",			"off"},
		{"Headset Playback Volume",		"0"},
		{"HF Left Playback",			"HF DAC"},
		{"HF Right Playback",			"HF DAC"},
		{"Handsfree Playback Volume",	"23"},
		/* ABE */
		{"DL1 PDM Switch",				"on"},
		{"DL2 Left Equalizer",				"0"},
		{"DL2 Right Equalizer",			"0"},
		{"DL2 Mixer Voice",				"on"},
		{"DL2 Voice Playback Volume",	"110"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Main Mic"},
		{"Analog Right Capture Route",	"Sub Mic"},
		{"Capture Preamplifier Volume",	"1"},
		{"Capture Volume",				"4"},
		/* ABE */
		{"AMIC_UL PDM Switch",			"on"},
		{"MUX_VX0",					"AMic0"},
		{"MUX_VX1",					"AMic1"},
		
		{"AUDUL Voice UL Volume",		"120"},
		{"Voice Capture Mixer Capture",	"on"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig headsetAudioConfig[] = {
	/* Downlink */
		/* TWL6040 */
		{"Earphone Driver Switch",		"off"},
		{"Earphone Playback Volume",		"0"},
		{"HF Left Playback",			"off"},
		{"HF Right Playback",			"off"},
		{"Handsfree Playback Volume",	"0"},
		{"HS Left Playback",			"HS DAC"},
		{"HS Right Playback",			"HS DAC"},
		{"Headset Playback Volume",		"13"},
		/* ABE */
		{"DL1 PDM Switch",				"on"},
		{"Sidetone Mixer Playback",		"on"},
		{"SDT DL Volume",				"120"},
		{"DL1 Mixer Voice",				"on"},
		{"DL1 Voice Playback Volume",	"110"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"Headset Mic"},
		{"Analog Right Capture Route",	"Headset Mic"},
		{"Capture Preamplifier Volume",	"1"},
		{"Capture Volume",				"4"},
		/* ABE */
		{"AMIC_UL PDM Switch",			"on"},
		{"MUX_VX0",					"AMic0"},
		{"MUX_VX1",					"AMic1"},
		
		{"AUDUL Voice UL Volume",		"120"},
		{"Voice Capture Mixer Capture",	"on"},
	/* Sidetone */
		{"Sidetone Mixer Capture",		"on"},
		{"SDT UL Volume",				"90"},
	/* eof */
		{ "eof", ""},
};

const struct audioConfig offAudioConfig[] = {
	/* Downlink */
		/* TWL6040 */
		{"Earphone Driver Switch",		"off"},
		{"Earphone Playback Volume",		"0"},
		{"HS Left Playback",			"off"},
		{"HS Right Playback",			"off"},
		{"Headset Playback Volume",		"0"},
		{"HF Left Playback",			"off"},
		{"HF Right Playback",			"off"},
		{"Handsfree Playback Volume",	"0"},
		/* ABE */
		{"DL1 PDM Switch",				"off"},
		{"Sidetone Mixer Playback",		"off"},
		{"SDT DL Volume",				"0"},
		{"DL1 Mixer Voice",				"off"},
		{"DL1 Voice Playback Volume",	"0"},
	/* Uplink */
		/* TWL6040 */
		{"Analog Left Capture Route",		"off"},
		{"Analog Right Capture Route",	"off"},
		{"Capture Preamplifier Volume",	"off"},
		{"Capture Volume",				"off"},
		/* ABE */
		{"AMIC_UL PDM Switch",			"off"},
		{"MUX_VX0",					"None"},
		{"MUX_VX1",					"None"},
		
		{"AUDUL Voice UL Volume",		"0"},
		{"Voice Capture Mixer Capture",	"off"},
	/* Sidetone */
		{"Sidetone Mixer Capture",		"off"},
		{"SDT UL Volume",				"0"},
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
		   "  -D, --device=NAME		select PCM by name (default hw:0,5)\n"
		   "  -c, --channels=#		channels (default 1)\n"
		   "  -p, --power=NAME		select power level (Low or High, default=Low)\n"
		   "  -s, --sidetone=#		enable(1)/disable(0) sidetone (default=1)\n"
		   "  -C, --command=NAME		select amixer command name (default alsa_amixer)");
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
	static const char short_options[] = "hvm:V:r:D:c:p:s:C:d";
	static const struct option long_options[] = {
		{ .name = "help", .val = 'h' },
		{ .name = "version", .val = 'v' },
		{ .name = "mode", .has_arg = 1, .val = 'm' },
		{ .name = "volume", .has_arg = 1, .val = 'V' },
		{ .name = "rate", .has_arg = 1, .val = 'r' },
		{ .name = "device", .has_arg = 1, .val = 'D' },
		{ .name = "channels", .has_arg = 1, .val = 'c' },
		{ .name = "power", .has_arg = 1, .val = 'p' },
		{ .name = "sidetone", .has_arg = 1, .val = 's' },
		{ .name = "command", .has_arg = 1, .val = 'C' },
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
			pcmName = optarg;
			break;
		case 'p':
			powerLevel = optarg;
			break;
		case 's':
			sideTone = strtol(optarg, NULL, 0);
			break;
		case 'C':
			alsaControlCommandName = optarg;
			break;
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
	char command[COMMAND_SIZE];

	if (debugFlag)
		sprintf(command, "%s -d cset name=\"%s\" \"%s\"", alsaControlCommandName, name, value);
	else
		sprintf(command, "%s -q cset name=\"%s\" \"%s\"", alsaControlCommandName, name, value);

	DEBUG("%s: system call: '%s'\n", __FUNCTION__, &command[0]);
#ifndef HOSTMODE
	error = system(&command[0]);
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
				error = setControlName(config->name, "off");
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

int startPcm(void)
{
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
}

int stopPcm(void)
{
	int error = 0;
	DEBUG("%s:  PCM name: %s sampleRate: %d channels: %d\n", __FUNCTION__, pcmName, sampleRate, channels);
#ifndef HOSTMODE
	SND_ERROR_RETURN(snd_pcm_drop(captureHandle));
	SND_ERROR_RETURN(snd_pcm_drop(playbackHandle));

	SND_ERROR_RETURN(snd_pcm_close(captureHandle));
	SND_ERROR_RETURN(snd_pcm_close(playbackHandle));
#endif
	return error;
}

int main(int argc ,char *argv[])
{
	int error = 0;

#ifdef HOSTMODE
	puts("!!!!running in host mode!!!!");
#endif
	if (argc == 1) {
		show_help();
		exit(EXIT_SUCCESS);
	}

	initSig();
	parse_options(argc, argv);

	DEBUG("audio mode chosen: %d\n", mode);
#ifndef HOSTMODE
	SND_ERROR_EXIT(snd_lib_error_set_handler(&ALSAErrorHandler));
#endif
	switch (mode) {
		case HANDSET:
			SND_ERROR_EXIT(setAudioConfig(handsetAudioConfig));
			break;
		case HANDFREE:
			SND_ERROR_EXIT(setAudioConfig(handfreeAudioConfig));
			break;
		case HEADSET:
			SND_ERROR_EXIT(setAudioConfig(headsetAudioConfig));
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
	SND_ERROR_EXIT(setAudioConfig(offAudioConfig));
	puts("Audio Voice Call off\n");
	exit(EXIT_SUCCESS);
}

