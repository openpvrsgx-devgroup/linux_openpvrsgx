
# Usage aberm.sh [file]
# e.g. aberm.sh soc-core will switch debug ON when removing all components. 

if [ "$1" != "" ]; then
echo "Debug component unbinding/remove for" $1
dkdbg $1 on
fi

rmmod snd-soc-sdp4430
rmmod snd-soc-twl6040

rmmod snd-soc-omap-dmic
rmmod snd-soc-omap-mcbsp
rmmod snd-soc-omap-mcpdm
rmmod snd-soc-omap-abe
rmmod snd-soc-omap-abe-dsp
rmmod snd-soc-abe-hal
rmmod snd-soc-dmic
rmmod snd-soc-omap

rmmod snd-soc-omap4-hdmi
rmmod omapdss

rmmod snd-soc-core

if [ "$1" != "" ]; then
dkdbg $1 off
fi

