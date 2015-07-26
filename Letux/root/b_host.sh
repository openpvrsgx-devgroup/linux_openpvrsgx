#!/bin/sh
# if not in software-controlled charging mode, Vbus (and only that)
# has to be applied before running the script
#MUSB_MODE=/sys/bus/platform/devices/musb-hdrc/mode
#MUSB_MODE=/sys/bus/platform/devices/musb-hdrc.0/mode
MUSB_MODE=/sys/bus/platform/devices/musb-hdrc.1.auto/mode

MUSB_M=`cat $MUSB_MODE`
echo "$MUSB_M"
if [ "$MUSB_M" != b_idle ] ; then
	echo Invalid mode at start
        exit
fi
# trigger a transition from b_idle to b_peripheral by simulating
# a connection to a host (enable pull-down resistors)
echo on > /sys/bus/platform/devices/twl4030_usb/fake_host
sleep 0.2
cat $MUSB_MODE
# trigger a transition from b_peripheral to b_hnp_enable
echo host >$MUSB_MODE
cat $MUSB_MODE
sleep 0.2
# simulate a disconnection from host
echo off > /sys/bus/platform/devices/twl4030_usb/fake_host
sleep 0.2
cat $MUSB_MODE
# be a host by ourselfs (if we wouldn't mess with this bit,
# it would be already set here)
echo on > /sys/bus/platform/devices/twl4030_usb/fake_host
MUSB_M=`cat $MUSB_MODE`
echo "$MUSB_M"

if [ "$MUSB_M" = b_wait_acon ] ; then
	echo "Prepared for being usb host, time to attach your device"
fi
