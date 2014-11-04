/**********************************************************************
 si4721 userspace quickhack - Copyright (C) 2012 - Andreas Kemnade
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied
 warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 ***********************************************************************/

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#define I2CADDR 0x11

static char *bus="/dev/i2c-2";
static int fd=-1;
/* sends a command to the si4721 chip and gets a response */
static int send_cmd(unsigned char *cmd_data,int cmd_len,unsigned char *resp_data, int resp_len)
{
	if(fd < 0) {
		fd=open(bus,O_RDWR);
		if (fd<0) {
			fprintf(stderr," cannot open %s\n", bus);
			exit(1);
		}
	}
#if 0
	struct i2c_rdwr_ioctl_data iod;
	struct i2c_msg msgs[2];
	if (resp_data)
		iod.nmsgs=2;
	else
		iod.nmsgs=1;
	iod.msgs=msgs;
	msgs[0].addr=I2CADDR;
	msgs[0].flags=0;
	msgs[0].buf=cmd_data;
	msgs[0].len=cmd_len;
	msgs[1].addr=I2CADDR;
	msgs[1].flags=I2C_M_RD;
	msgs[1].buf=resp_data;
	msgs[1].len=resp_len;
	if((resp_len=ioctl(fd,I2C_RDWR,&iod)) < 0)	// does this really return the response length?
		fprintf(stderr, "I2C error: %s\n", strerror(errno));
#else
	if (ioctl(fd, I2C_SLAVE, I2CADDR) < 0) {
		printf("Failed to acquire bus access and/or talk to slave: %s\n", strerror(errno));
		return -1;
	}
	if(write(fd,cmd_data,cmd_len) != cmd_len) {
		printf("Write error: %s\n", strerror(errno));
		return -1;
	}
	// here we should wait for the interrupt (1us impulse on GPO2/INT connected to CLKR/GPIO156) and potentially poll for STC (seek)
	usleep(110*1000);	/* give the MCU in the Si4721 some time to prepare the response */
	if(resp_data != NULL && read(fd,resp_data,resp_len) < 0) {
		printf("Read error: %s\n", strerror(errno));
		return -1;
	}
	// here we should wait for the interrupt (1us impulse on GPO2/INT connected to CLKR/GPIO156)
	usleep(100*1000);	/* give the MCU in the Si4721 some time to prepare for the next command */
#endif
	return resp_len;
}

void setprop(int number, int value)	// should be u16...
{
	unsigned char prop[5];
	unsigned char resp[1];
	prop[0]=0x12;
	prop[1]=0;
	prop[2]=number/256;
	prop[3]=number%256;
	prop[4]=value/256;
	prop[5]=value%256;
	send_cmd(prop,6,resp,1);
	if(resp[0]&0x40)
		printf("error setting property %04x to %04x\n", number, value);
}

void usage(char **argv)
{
	fprintf(stderr,"Usage: %s [-i /dev/i2c-X] [-d] { -up | -dn | -t freq-in-10khz | -r srate | -ts | -s }\n");
	fprintf(stderr," e.g.: %s -i /dev/i2c-2 -up -t 9380 for 93800khz\n",argv[0]);
	fprintf(stderr,"   or: %s -dn to power off\n",argv[0]);
	exit(1);
	}

int main(int argc, char **argv)
{
	unsigned char resp[16];
	int i;
	int debug=0;
	while(argc > 1 && argv[1][0] == '-')
		{
		if(strcmp(argv[1], "-help") == 0)
			usage(argv);
		else if(strcmp(argv[1], "-d") == 0)
			debug=1, argv++, argc--;
		else if(strcmp(argv[1], "-up") == 0)
			{ /* power up */
				send_cmd("\x01\x00\xb0",3,resp,1);
				if(debug) {
					/* get revision */
					printf("init resp: %02x\n",(int)resp[0]);
					send_cmd("\x10",1,resp,9);
					printf("get_chiprev resp: %02x\n",(int)resp[0]);
					for(i=1;i<9;i++) {
						printf("%02x",resp[i]);
					}
					printf("\n");
				}
				argv++, argc--;
			}
		else if(strcmp(argv[1], "-dn") == 0)
			{ /* power down */
				send_cmd("\x11",1,resp,1);
				printf("powered off\n");
				argv++, argc--;
			}
		else if(strcmp(argv[1], "-t") == 0)
			{ /* tune */
				int freq;
				unsigned char tune[5];
				if(argc <= 2)
					usage(argv);
				freq=atoi(argv[2]);
				if(debug)
					printf("freq: %d\n", freq);
				tune[0]=0x20;
				tune[1]=0;
				tune[2]=freq/256;
				tune[3]=freq%256;
				tune[4]=0;
				send_cmd(tune,5,resp,1);
				if(debug)
					printf("tune freq: %02x\n",(int)resp[0]);
				if (resp[0]&0x40)
					printf("invalid frequency (6400 .. 10800)!\n");
				argv+=2, argc-=2;
			}

		else if(strcmp(argv[1], "-ts") == 0)
			{ /* tune status */
				send_cmd("\x22\x00",2,resp,8);
				if(debug)
					printf("status resp: %02x %02x\n",(int)resp[0],(int)resp[1]);
				printf("tuned to %d0khz RSSI %ddBuV SNR %ddB",((int)resp[2])*256+resp[3], (int)resp[4],(int)resp[5]);
				if (resp[1]&1)
					printf(" valid\n");
				else
					printf(" weak\n");
				if(debug) {
					for(i=1;i<8;i++) {
						printf("%02x",resp[i]);
					}
					printf("\n");
				}
				argv++, argc--;
			}
		else if(strcmp(argv[1], "-s") == 0)
			{ /* signal status */
				send_cmd("\x23\x00",2,resp,8);
				if(debug)
					printf("status resp: %02x %02x\n",(int)resp[0],(int)resp[1]);
				printf("RSSI %ddBuV SNR %ddB offset %dkHz",(int)resp[4],(int)resp[5],(int)(signed char)resp[7]);
				/* default parameter values expect RSSI level of 20dB for "VALID" and 39dB for full HiFi stereo */
				if (resp[3]&0x80)
					printf(" stereo");
				if (resp[2]&1)
					printf(" valid\n");
				else
					printf(" weak\n");
				if(debug) {
					for(i=1;i<8;i++) {
						printf("%02x",resp[i]);
					}
					printf("\n");
				}
				argv++, argc--;
			}
		else if(strcmp(argv[1], "-rds") == 0)
			{ /* RDS status */
				setprop(0x1502, 1);	// enable
				setprop(0x1501, 1);	// interrupt after first RDS group in FIFO
				send_cmd("\x24\x00",2,resp,13);
				if(debug)
					printf("status resp: %02x %02x\n",(int)resp[0],(int)resp[1]);
				printf("FIFO %d",(int)resp[3]);
				if (resp[1]&4)
					printf(" found");
				if (resp[2]&1)
					printf(" sync");
				printf("\n");
				if(debug) {
					for(i=1;i<13;i++) {
						printf("%02x",resp[i]);
					}
					printf("\n");
				}
				argv++, argc--;
			}
		else if(strcmp(argv[1], "-r") == 0)
			{ /* rate */
				int rate;
				unsigned char prop[5];
				if(argc <= 2)
					usage(argv);
				rate=atoi(argv[2]);
				if(debug)
					printf("rate: %d\n", rate);
#if 0
				prop[0]=0x12;
				prop[1]=0;
				// should we set prop 0201 and 0202?
				prop[2]=0x0102/256;	// property 0xx0102
				prop[3]=0x0102%256;
				prop[4]=128/256;
				prop[5]=128&256;
				send_cmd(prop,6,NULL,0);
				prop[2]=0x0104/256;	// property 0xx0104
				prop[3]=0x0104%256;
				prop[4]=rate/256;
				prop[5]=rate%256;
				send_cmd(prop,6,NULL,0);
				// handle errors
#else
				setprop(0x0102, 128);
				setprop(0x0104, rate);
#endif
				argv+=2, argc-=2;
			}
		else if(strcmp(argv[1], "-i") == 0)
			{ /* choose i2c bus */
				if(argc <= 2)
					usage(argv);
				bus=argv[2];
				argv+=2, argc-=2;
			}
		else
			usage(argv);
		}
	return 0;
}
