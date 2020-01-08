#!/bin/bash

# assume we have a native and working panel driver and need to rotate by tiler
/root/tiler-ctl 270
# setup keyboard
cp /etc/X11/Xmodmap.pyra /etc/X11/Xmodmap
# setup nubs
echo mouse > /proc/pandora/nub0/mode	# mouse
echo scroll > /proc/pandora/nub1/mode	# scroll wheel
# mouse buttons are handled by gpio-buttons
