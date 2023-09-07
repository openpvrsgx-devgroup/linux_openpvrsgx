#! /bin/bash
#
# configure host for tethering with an Letux device connected through ethernet over USB (RNDIS/ethernet gadget)
# it uses the current service or default route for internet connection
# on Mac the device is identified by its IP address
# on Linux the device is (currently) assumed to create ifconfig usb0
#

DEVICE=192.168.0.202	# Letux device

case "$(uname -s)" in
Darwin )

# based on
# http://apple.stackexchange.com/questions/228936/using-server-5-0-15-to-share-internet-without-internet-sharing
# https://superuser.com/questions/931827/sharing-openvpn-on-mac-os-x-yosemite

INTERFACE=""

# VPN hides highest priority active network

while read LINE
do
	NAME=$(echo $LINE | awk -F  "(, )|(: )|[)]" '{print $2}')
	DEV=$(echo $LINE | awk -F  "(, )|(: )|[)]" '{print $4}')
	# echo "Current service: $NAME, $DEV, $CURRENT, $INTERFACE"
	if [ "$DEV" ]
	then
		if [ "$NAME" = utun0 ] && ifconfig "$DEV" 2>/dev/null | grep -q 'status: active'
		then
			CURRENT="$NAME"
			INTERFACE="$DEV"
			break 2	# take first we find
		fi
	fi
done < <(networksetup -listnetworkserviceorder | grep 'Hardware Port')

if [ ! "$INTERFACE" ]
then
# locate highest priority active service
# found at https://apple.stackexchange.com/questions/191879/how-to-find-the-currently-connected-network-service-from-the-command-line
# and improved...

	while read LINE
	do
		NAME=$(echo $LINE | awk -F  "(, )|(: )|[)]" '{print $2}')
		DEV=$(echo $LINE | awk -F  "(, )|(: )|[)]" '{print $4}')
		# echo "Current service: $NAME, $DEV, $CURRENT, $INTERFACE"
		if [ "$DEV" ]
		then
			if ifconfig "$DEV" 2>/dev/null | grep -q 'status: active'
			then
				CURRENT="$NAME"
				INTERFACE="$DEV"
				break 2	# take first we find
			fi
		fi
	done < <(networksetup -listnetworkserviceorder | grep 'Hardware Port')

fi

if [ -n "$CURRENT" ]
then
	echo Sharing over: $CURRENT $INTERFACE
else
	echo "Could not find current service" >&2
	exit 1
fi

case "$(uname -r)" in
21.* | 18.* | 15.* )

echo "(ignore messages about ALTQ)"
sudo pfctl -F nat

echo "nat log on $INTERFACE from $DEVICE to any -> ($INTERFACE)"
#nat log on en0 from $DEVICE to any -> (en0)" | sudo pfctl -N -f - -e
echo "(ignore messages about -f option flushing rules)"
echo "nat log on $INTERFACE from $DEVICE to any -> ($INTERFACE)" | sudo pfctl -N -f - -e
sudo pfctl -a '*' -s nat

echo "sysctl (one of these should succeed)"	# FIXME: we could make this depend on $(uname -r)
sudo sysctl -w net.inet.ip.forwarding=1
sudo sysctl -w net.inet.ip.fw.enable=1
# sudo sysctl -w net.inet6.ip6.forwarding=1

## logging
# ifconfig pflog1 create
# sudo tcpdump -n -e -ttt -i pflog1
# sudo tcpdump -n -e -ttt -i $INTERFACE
;;
esac
;;

Linux )

# based on http://jpdelacroix.com/tutorials/sharing-internet-beaglebone-black.html

HOST=192.168.0.200
IF=usb0	# fixme - find out where the Letux device is connected to
INTERFACE=$(route | grep '^default' | head -1 | grep -o '[^ ]*$')	# first default route

if [ ! "$INTERFACE" ]
then
	echo "Could not find current internet service interface" >&2
	exit 1
fi

echo Sharing over: $INTERFACE

ifconfig $IF $HOST
sysctl net.ipv4.ip_forward=1
iptables --table nat --append POSTROUTING --out-interface $INTERFACE -j MASQUERADE
iptables --append FORWARD --in-interface $IF -j ACCEPT

;;

* )
echo unknown operating system $(uname -a)
;;
esac
