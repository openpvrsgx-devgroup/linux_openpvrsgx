#!/bin/bash
# detect Openmoko wall charger and increase input current limit

case "$1" in
"pyra" )
	# if wwan is now on (and usb is charging) => limit system voltage
	# check if USB is connected? cat /sys/class/extcon/extcon0/cable.0/state
	# check if USB-host is connected? cat /sys/class/extcon/extcon0/cable.1/state
	BQ24297=/sys/class/power_supply/bq24297
	RFKILL_STATE=
	USB_STATE=

	if rfkill list wwan | fgrep "Soft blocked: no" -q
	then # limit max. charging voltage
		echo 4050000
	else # unlimit voltage
		echo 4200000
	fi >$BQ24297/constant_charge_voltage_max

	echo $(date) "$SUBSYSTEM $ACTION" $(cat $BQ24297/constant_charge_voltage_max 2>/dev/null) >>/tmp/charger.log

	;;

gta04 | * )
	# FIXME: replace /dev/usb_id mechanism by access to /sys/class/power_supply/twl4030 or similar

	case $(cat /dev/usb_id 2>/dev/null) in
		"floating" ) echo "500000" >/dev/usb_max_current;;
		"102k" ) echo "851000" >/dev/usb_max_current;;
		"GND" ) echo "851000" >/dev/usb_max_current;;
	esac

	# remove # to enable
	echo $(date) "$SUBSYSTEM $ACTION" $(cat /dev/usb_id 2>/dev/null) $(cat /dev/usb_max_current 2>/dev/null) >>/tmp/charger.log
	;;

esac
