
# Usage abeld.sh [file]
# e.g. abeld.sh soc-core will switch debug ON when probing all components. 

if [ "$1" != "" ]; then
echo "Debug component binding/probe for" $1
modprobe snd-soc-core
dkdbg $1 on
fi

modprobe snd-soc-omap
modprobe snd-soc-omap-dmic
modprobe snd-soc-omap-mcbsp
modprobe snd-soc-omap-mcpdm
modprobe snd-soc-omap-abe
modprobe snd-soc-omap-abe-dsp
modprobe snd-soc-dmic
modprobe snd-soc-twl6040
modprobe snd-soc-abe-hal
modprobe snd-soc-sdp4430
modprobe snd-soc-omap4-hdmi

if [ "$1" != "" ]; then
dkdbg $1 off
fi
