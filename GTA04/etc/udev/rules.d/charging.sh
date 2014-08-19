#!/bin/bash

case $(cat /dev/usb_id 2>/dev/null) in
	"floating" ) echo "500000" >/dev/usb_max_current;;
	"102k" ) echo "851000" >/dev/usb_max_current;;
	"GND" ) echo "851000" >/dev/usb_max_current;;
esac

# remove # to enable
# echo $(date) "$SUBSYSTEM $ACTION" $(cat /dev/usb_id 2>/dev/null) $(cat /dev/usb_max_current 2>/dev/null) >>/tmp/charger.log
