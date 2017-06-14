#!/bin/sh
echo 0 >/sys/class/vtconsole/vtcon1/bind
modprobe -r omapdrm
modprobe drm_kms_helper fbdev_rotation=8
modprobe omapdrm
cat /dev/dri/card0 > /dev/null &
