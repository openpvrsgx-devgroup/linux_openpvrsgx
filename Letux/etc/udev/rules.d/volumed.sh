#!/bin/bash
# start volume monitoring

if [ ! -d /root/pyra_vol_mon ]
then
	echo pyra_vol_mon not installed >&2
	exit
fi

killall pyra_vol_mon pyra_vol_ctl

if [ "$(/root/findiio palmas-gpadc)" -a "$(/root/findsoundcard L15)" ]
then # if both are or became available, start volume monitor in background
	( cd /root/pyra_vol_mon && make )	# rebuild if needed
#	/root/pyra_vol_mon/pyra_vol_mon -c 2 -l 0 -u 1200 echo &
	/root/pyra_vol_mon/pyra_vol_mon -c 2 -l 0 -u 1200 /root/pyra_vol_mon/pyra_vol_ctl &
fi
