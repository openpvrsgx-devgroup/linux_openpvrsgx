#!/bin/bash

if [ -r /sys/bus/i2c/devices/1-0048 ]
then	# old V3 touch&display adapter for display demo/tester
	/root/boe-w677l
else
	/root/ssd2858 -r -p -s	# start ssd
fi

/root/bl 1
