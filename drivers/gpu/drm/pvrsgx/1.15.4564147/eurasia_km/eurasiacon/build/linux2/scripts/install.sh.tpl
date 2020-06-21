#!/bin/bash
############################################################################ ###
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Dual MIT/GPLv2
# 
# The contents of this file are subject to the MIT license as set out below.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# Alternatively, the contents of this file may be used under the terms of
# the GNU General Public License Version 2 ("GPL") in which case the provisions
# of GPL are applicable instead of those above.
# 
# If you wish to allow use of your version of this file only under the terms of
# GPL, and not to allow others to use your version of this file under the terms
# of the MIT license, indicate your decision by deleting the provisions above
# and replace them with the notice and other provisions required by GPL as set
# out in the file called "GPL-COPYING" included in this distribution. If you do
# not delete the provisions above, a recipient may use your version of this file
# under the terms of either the MIT license or GPL.
# 
# This License is also included in this distribution in the file called
# "MIT-COPYING".
# 
# EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
# PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#### ###########################################################################

SCRIPT_ROOT=`dirname $0`

# PVR Consumer services version number
#
PVRVERSION=[PVRVERSION]

# Exit with an error messages.
# $1=blurb
#
bail()
{
    echo "$1" >&2
    echo "" >&2
    echo "Installation failed" >&2
    exit 1
}

# basic installation function
# $1=fromfile, $2=destfilename, $3=blurb, $4=chmod-flags, $5=chown-flags
#
install_file()
{
	DESTFILE=${DISCIMAGE}$2
	DESTDIR=`dirname $DESTFILE`

	if [ ! -e $1 ]; then
	 	[ -n "$VERBOSE" ] && echo "skipping file $1 -> $2"
		 return
	fi
	
	# Destination directory - make sure it's there and writable
	#
	if [ -d "${DESTDIR}" ]; then
		if [ ! -w "${DESTDIR}" ]; then
			bail "${DESTDIR} is not writable."
		fi
	else
		$DOIT mkdir -p ${DESTDIR} || bail "Couldn't mkdir -p ${DESTDIR}"
		[ -n "$VERBOSE" ] && echo "Created directory `dirname $2`"
	fi

	# Delete the original so that permissions don't persist.
	#
	$DOIT rm -f $DESTFILE

	$DOIT cp -f $1 $DESTFILE || bail "Couldn't copy $1 to $DESTFILE"
	$DOIT chmod $4 ${DESTFILE}
	$DOIT chown $5 ${DESTFILE}

	echo "$3 `basename $1` -> $2"
	$DOIT echo "file $2" >>${DDK_INSTALL_LOG}
}

# Install a symbolic link
# $1=fromfile, $2=destfilename
#
install_link()
{
	DESTFILE=${DISCIMAGE}$2
	DESTDIR=`dirname $DESTFILE`

	if [ ! -e ${DESTDIR}/$1 ]; then
		 [ -n "$VERBOSE" ] && echo $DOIT "skipping link ${DESTDIR}/$1"
		 return
	fi

	# Destination directory - make sure it's there and writable
	#
	if [ -d "${DESTDIR}" ]; then
		if [ ! -w "${DESTDIR}" ]; then
			bail "${DESTDIR} is not writable."
		fi
	else
		$DOIT mkdir -p ${DESTDIR} || bail "Couldn't mkdir -p ${DESTDIR}"
		[ -n "$VERBOSE" ] && echo "Created directory `dirname $2`"
	fi

	SONAME=`objdump -p ${DESTDIR}/$1 | grep SONAME | awk '{print $2}'`
	
	if [ -n "$SONAME" ]; then
		$DOIT ln -sf $1 ${DESTDIR}/$SONAME || bail "Couldn't link $1 to $SONAME"
		$DOIT echo "link ${DESTDIR}/$SONAME" >>${DDK_INSTALL_LOG}
	fi
	
	BASENAME=`expr match $1 '\(.\+\.so\)'`
	
	if [ "$BASENAME" != "$1" ]; then
		$DOIT ln -sf $1 ${DESTDIR}/$BASENAME || bail "Couldn't link $1 to $BASENAME"
		$DOIT echo "link ${DESTDIR}/$BASENAME" >>${DDK_INSTALL_LOG}
	fi
}

# Tree-based installation function
# $1 = fromdir $2=destdir $3=blurb
#
install_tree()
{
	# Make the destination directory if it's not there
	#
	if [ ! -d ${DISCIMAGE}$2 ]; then
		$DOIT mkdir -p ${DISCIMAGE}$2 || bail "Couldn't mkdir -p ${DISCIMAGE}$2"
	fi
	if [ "$DONTDOIT" ]; then
		echo "### tar -C $1 -cf - . | tar -C ${DISCIMAGE}$2 -x${VERBOSE}f -" 
	else
		tar -C $1 -cf - . | tar -C ${DISCIMAGE}$2 -x${VERBOSE}f -
	fi
	if [ $? = 0 ]; then
		echo "Installed $3 in ${DISCIMAGE}$2"
		$DOIT echo "tree $2" >>${DDK_INSTALL_LOG}
	else
		echo "Failed copying $3 from $1 to ${DISCIMAGE}$2"
	fi
}

# Uninstall something.
#
uninstall()
{
	if [ ! -f ${DDK_INSTALL_LOG} ]; then
		echo "Nothing to un-install."
		return;
	fi

	BAD=0
	VERSION=""
	while read type data; do
		case $type in
		version)	# do nothing
			echo "Uninstalling existing version $data"
			VERSION="$data"
			;;
		link|file) 
			if [ -z "$VERSION" ]; then
				BAD=1;
				echo "No version record at head of ${DDK_INSTALL_LOG}"
			elif ! $DOIT rm -f ${DISCIMAGE}${data}; then
				BAD=1;
			else
				[ -n "$VERBOSE" ] && echo "Deleted $type $data"
			fi
			;;
		tree)		# so far, do nothing
			;;
		esac
	done < ${DDK_INSTALL_LOG};

	if [ $BAD = 0 ]; then
		echo "Uninstallation completed."
		$DOIT rm -f ${DDK_INSTALL_LOG}
	else
		echo "Uninstallation failed!!!"
	fi
}

# Help on how to invoke
#
usage()
{
	echo "usage: $0 [options...]"
	echo ""
	echo "Options: -v            verbose mode"
	echo "         -n            dry-run mode"
	echo "         -u            uninstall-only mode"
	echo "         --no-x        don't install X window system"
	echo "         --no-display  don't install integrated PowerVR display module"
	echo "         --root path   use path as the root of the install file system"
	exit 1
}

# Work out if there are any special instructions.
#
while [ "$1" ]; do
	case "$1" in
	-v|--verbose)
		VERBOSE=v
		;;
	-r|--root)
		DISCIMAGE=$2
		shift;
		;;
	-u|--uninstall)
		UNINSTALL=y
		;;
	-n)	
		DOIT=echo
		;;
	--no-x)
		NO_X=y
		;;
	--no-display)
		NO_DISPLAYMOD=y
		;;
	-h | --help | *)	
		usage
		;;
	esac
	shift
done

# Find out where we are?  On the target?  On the host?
# If we are running on an ARM device, assume we are on the target, and
# set DISCIMAGE appropriately.
case `uname -m` in
arm*)	
	host=0
	from=target
	DISCIMAGE=/
	;;
sh*)
	host=0
	from=target
	DISCIMAGE=/
	;;
i?86*)
	host=1
	from=host
	;;
x86_64*)
	host=1
	from=host
	;;
*)
	bail "Don't know host to perform on machine type `uname -m`"
	;;
esac

if [ -z "$DISCIMAGE" ]; then	
	bail "DISCIMAGE must be set for installation to be possible."
fi

if [ ! -d "$DISCIMAGE" ]; then
	bail "$0: $DISCIMAGE does not exist."
fi

echo
echo "Installing PowerVR '$PVRVERSION' on $from"
echo
echo "File system installation root is $DISCIMAGE"
echo

OLDLOG=$DISCIMAGE/etc/powervr_ddk_install.log
KMLOG=$DISCIMAGE/etc/powervr_ddk_install_km.log
UMLOG=$DISCIMAGE/etc/powervr_ddk_install_um.log

# Uninstall anything using the old-style install scripts.
if [ -f $OLDLOG ]; then
    if [ -f $KMLOG -a $KMLOG -nt $OLDLOG ]; then
        # Last install was new scheme.
        rm $OLDLOG
    elif [ -f $UMLOG -a $UMLOG -nt $OLDLOG ]; then
        # Last install was new scheme.
        rm $OLDLOG
    else
        echo "Uninstalling all components from legacy log."
        DDK_INSTALL_LOG=$OLDLOG
        uninstall
        echo 
        LEGACY_LOG=1
    fi
fi

# Install KM components
if [ -f $SCRIPT_ROOT/install_km.sh ]; then
    DDK_INSTALL_LOG=$KMLOG
    if [ -z "$LEGACY_LOG" -a -f $KMLOG ]; then
        echo "Uninstalling Kernel components"
        uninstall
        echo 
    fi
    if [ -z "$UNINSTALL" ]; then
        echo "Installing Kernel components"
        $DOIT echo "version $PVRVERSION" > ${DDK_INSTALL_LOG}
        source $SCRIPT_ROOT/install_km.sh
    fi
    echo
fi

# Install UM components
if [ -f $SCRIPT_ROOT/install_um.sh ]; then
    DDK_INSTALL_LOG=$UMLOG
    if [ -z "$LEGACY_LOG" -a -f $UMLOG ]; then
        echo "Uninstalling User components"
        uninstall
        echo 
    fi
    if [ -z "$UNINSTALL" ]; then
        echo "Installing User components"
        $DOIT echo "version $PVRVERSION" > ${DDK_INSTALL_LOG}
        source $SCRIPT_ROOT/install_um.sh
    fi
    echo 
fi

# Create an OLDLOG so old versions of the driver can uninstall.
$DOIT echo "version $PVRVERSION" > ${OLDLOG}
if [ -f $KMLOG ]; then
	tail -n +2 $KMLOG >> $OLDLOG
fi
if [ -f $UMLOG ]; then
	tail -n +2 $UMLOG >> $OLDLOG
fi

# Make sure new logs are newer than $OLDLOG
touch -m -d "last sunday" $OLDLOG

# All done...
#
echo "Installation complete!"
if [ "$host" = 0 ]; then
   echo "You may now reboot your target."
fi
echo
