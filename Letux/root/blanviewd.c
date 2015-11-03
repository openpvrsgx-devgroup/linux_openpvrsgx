/*
 * blanviewd.c
 *
 * daemon to read the ambient light sensor and control
 * backlight intensity for Ortustech Blanview display
 *
 * (c) H. N. Schaller, Golden Delicious Comp. GmbH&Co. KG, 2013
 *     Lukas Märdian <lukas@goldelico.com>
 * Licence: GNU GPL2
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	char debug=0;
	int prev_bl=-1;
	char *file;
	FILE *in, *out;
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
	file="/sys/class/backlight/backlight/brightness";
#else
	file="/sys/class/backlight/backlight/max_brightness";
#endif
	out=fopen(file, "w");
	if(!out)
		{
		fprintf(stderr, "%s: %s\n", file, strerror(errno));
		return 1;
		}
	while(1)
		{
		int i, n;
		unsigned short aux;
		unsigned short ambient_light;
		unsigned short bl;
		file="/sys/bus/iio/devices/iio:device1/in_voltage4_raw";
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
				ambient_light = 1;
			else if(aux > 1024)
				ambient_light = 2;
			else if(aux > 512)
				ambient_light = 3;
			else if(aux > 256)
				ambient_light = 4;
			else 	// min ambient light
				ambient_light = 5;

			bl = ambient_light*20;
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
