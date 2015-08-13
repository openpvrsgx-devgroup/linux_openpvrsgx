#!/bin/bash
#
# this is a snippet of kernel-adaptation-n900.spec,
# which is part of the distribution Mer (merproject.org).
# https://build.merproject.org/package/view_file/nemo:devel:hw:ti:omap3:n900/kernel-adaptation-n900/kernel-adaptation-n900.spec
# Copied on 07 April 2015
#
# Original Authors unknown
# License unknown
#

# Script to install kernel headers
# License: MIT (or original license if incompatible)
#
# Copyright: 0000-2013 Original Authors
# Copyright: 2015 Josua Mayer
#

usage() {
	echo "$0 <architecture> <destdir>"
}

if [ "x$#" != "x2" ]; then
	usage
	exit 1
fi

KARCH=$1
DESTDIR="$2"

# This all looks scary, but the end result is supposed to be:
# * all arch relevant include/ files
# * all Makefile/Kconfig files
# * all script/ files

mkdir -p "${DESTDIR}"

# dirs for additional modules per module-init-tools, kbuild/modules.txt
# first copy everything
cp --parents `find  -type f -name "Makefile*" -o -name "Kconfig*"` "${DESTDIR}"
cp Module.symvers "${DESTDIR}"
cp System.map "${DESTDIR}"
if [ -s Module.markers ]; then
cp Module.markers "${DESTDIR}"
fi
# then drop all but the needed Makefiles/Kconfig files
rm -rf "${DESTDIR}/Documentation"
rm -rf "${DESTDIR}/scripts"
rm -rf "${DESTDIR}/include"

# Copy all scripts
cp .config "${DESTDIR}"
cp -a scripts "${DESTDIR}"
if [ -d arch/${KARCH}/scripts ]; then
cp -a arch/${KARCH}/scripts "${DESTDIR}/arch/${KARCH}"
fi
# FIXME - what's this trying to do ... if *lds expands to multiple files the -f test will fail.
if [ -f arch/${KARCH}/*lds ]; then
cp -a arch/${KARCH}/*lds "${DESTDIR}/arch/${KARCH}/"
fi
# Clean any .o files from the 'scripts'
find "${DESTDIR}/scripts/" -name \*.o -print0 | xargs -0 rm -f

# arch-specific include files
cp -a --parents arch/${KARCH}/include "${DESTDIR}"

# arm has include files under plat- and mach- areas (x86/mips don't)
if [ "x$KARCH" = "xarm" ]; then
	cp -a --parents arch/${KARCH}/mach-*/include "${DESTDIR}"
	cp -a --parents arch/${KARCH}/plat-*/include "${DESTDIR}"
fi

# normal include files
mkdir -p "${DESTDIR}/include"

# copy only include/* directories
cp -a $(find include -mindepth 1 -maxdepth 1 -type d) "${DESTDIR}/include"

# delete any installed .cmd files
find "${DESTDIR}/" -name '.*.cmd' -delete

# Make sure the Makefile and version.h have a matching timestamp so that
# external modules can be built. Also .conf
touch -r "${DESTDIR}/Makefile" "${DESTDIR}/include/generated/uapi/linux/version.h"
touch -r "${DESTDIR}/.config" "${DESTDIR}/include/generated/autoconf.h"

# Copy .config to include/config/auto.conf so "make prepare" is unnecessary.
cp "${DESTDIR}/.config" "${DESTDIR}/include/config/auto.conf"
