#!/bin/bash

[ -x /root/mount-boot ] && /root/mount-boot

case "$(uname -r)" in
	*-letux+ ) SUFFIX=;;
	*-letux-lpae+ ) SUFFIX=-lpae;;
	*-letux-arm64+ ) SUFFIX=-arm64;;
	*-letux-l400+ ) SUFFIX=-l400;;
	* ) echo "unknown machine $(uname -m)"; exit;;
esac

# SUFFIX=$(expr "$(uname -r)" : ".*-letux\([^+]*\)+")

wget -O - https://download.goldelico.com/letux-kernel/latest$SUFFIX/modules.tgz | (cd / && tar xvzf - --keep-directory-symlink)
wget -O - https://download.goldelico.com/letux-kernel/latest$SUFFIX/device-trees.tbz | (cd /boot && tar xvjf - --keep-directory-symlink)
wget -O /boot/uImage https://download.goldelico.com/letux-kernel/latest$SUFFIX/uImage

echo New kernel installed. Please reboot.
