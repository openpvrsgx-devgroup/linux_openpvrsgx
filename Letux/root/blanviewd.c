/*
 * blanviewd.c
 *
 * daemon to read the ambient light sensor and control
 * backlight intensity for Ortustech Blanview display
 *
 * (c) H. N. Schaller <hns@goldelico.com>
 *     Lukas Märdian <lukas@goldelico.com>
 *     Golden Delicious Comp. GmbH&Co. KG, 2013-16
 * Licence: GNU GPL2
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAX_BL 10	/* defined by device tree */

int main(int argc, char *argv[])
{
	char debug=0;
	int prev_bl=-1;
	char file[256];
	FILE *in=NULL, *out;
	int i, n;
	if(argv[1] != NULL && strcmp(argv[1], "-d") == 0)
		{
		fprintf(stderr, "debug mode\n");
		debug=1;
		}
	else if(argv[1])
		{
		fprintf(stderr, "usage: blanviewd [-d]\n");
		return 1;
		}
#if 1
	strcpy(file, "/sys/class/backlight/backlight/brightness");
#else
	strcpy(file, "/sys/class/backlight/backlight/max_brightness");
#endif
	out=fopen(file, "w");
	if(!out)
		{
		fprintf(stderr, "%s: %s\n", file, strerror(errno));
		return 1;
		}
	for(i=0; i<100; i++)
		{
		char c;
		sprintf(file, "/sys/bus/iio/devices/iio:device%d/name", i);
		if(debug)
			fprintf(stderr, "trying %s\n", file);
		in=fopen(file, "r");
		if(!in)
			break;
		n=fscanf(in, "tsc2007%c", &c);
		if(debug)
			fprintf(stderr, "n=%d\n", n);
		fclose(in);
		if(n == 1)
			break;	// found
		in=NULL;
		}
	if(!in)
		{
		fprintf(stderr, "no tsc2007 iio device found\n");
		return 1;
		}
	sprintf(file, "/sys/bus/iio/devices/iio:device%d/in_voltage4_raw", i);
	if(debug)
		fprintf(stderr, "iio device=%s\n", file);
	while(1)
		{
		unsigned short aux;
		unsigned short ambient_light;
		unsigned short bl;
		in=fopen(file, "r");
		if(!in)
			{
			fprintf(stderr, "%s: %s\n", file, strerror(errno));
			return 1;
			}
		n=fscanf(in, "%hu", &aux);
		fclose(in);
		if(n == 1)
			{
			if(debug)
				fprintf(stderr, "aux=%d", aux);

			if(aux > 2048)	// max ambient light
				ambient_light = 0;
			else if(aux > 1024)
				ambient_light = 1;
			else if(aux > 512)
				ambient_light = 2;
			else if(aux > 256)
				ambient_light = 3;
			else 	// min ambient light
				ambient_light = 4;

			bl = 1+((MAX_BL-1)*ambient_light)/4;
			if(debug)
				fprintf(stderr, " ->");
			while(prev_bl != bl)
				{
				if(prev_bl < 0)
					prev_bl=bl;	// first
				else if(prev_bl < bl)
					prev_bl+=1;
				else
					prev_bl-=1;
				fprintf(out, "%d\n", prev_bl);	// fade backlight level
				fflush(out);
				if(debug)
					fprintf(stderr, " %d", prev_bl);
				usleep(10000);
				}
			prev_bl = bl;
			if(debug)
				fprintf(stderr, "\n");
			}
		else if(debug)
			fprintf(stderr, "n=%d\n", n);	// driver mismatch...
		sleep(5); // next sample in 5 sec
		}
}
