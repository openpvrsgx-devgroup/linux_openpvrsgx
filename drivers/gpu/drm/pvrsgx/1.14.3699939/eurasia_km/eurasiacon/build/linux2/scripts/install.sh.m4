divert(-1)
############################################################################ ###
##@Title         Target install script
##@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
##@License       Strictly Confidential.
#### ###########################################################################

include(../scripts/common.m4)

###############################################################################
##
## Diversion discipline
##
## 49    variables
## 50    generic functions
## 51    function header of install_pvr function
## 52    body of install_pvr function
## 53    function trailer of install_pvr function
## 54    function header of install_X function
## 55    body of install_X function
## 56    function trailer of install_X function
## 80    script body
## 200   main case statement for decoding arguments.
##
###############################################################################

###############################################################################
##
## Diversion #49 - Variables that might be overridden
##
###############################################################################
pushdivert(49)dnl
# PVR Consumer services version number
#
[[PVRVERSION]]="PVRVERSION"

popdivert

define([[SET_DDK_INSTALL_LOG_PATH]], [[pushdivert(49)dnl
# Where we record what we did so we can undo it.
#
DDK_INSTALL_LOG=/etc/powervr_ddk_install.log
popdivert]])

###############################################################################
##
## Diversion #50 - Some generic functions
##
###############################################################################
pushdivert(50)dnl

[[# basic installation function
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
	if [ ! -e $1 ]; then
		[ -n "$VERBOSE" ] && echo "skipping file $1 -> $2"
		 return
	fi

	DESTFILE=${DISCIMAGE}$2
	DESTDIR=`dirname $DESTFILE`

	$DOIT mkdir -p ${DESTDIR} || bail "Couldn't mkdir -p ${DESTDIR}"
	[ -n "$VERBOSE" ] && echo "Created directory `dirname $2`"

	# Delete the original so that permissions don't persist.
	$DOIT rm -f $DESTFILE
	$DOIT cp -f $1 $DESTFILE || bail "Couldn't copy $1 to $DESTFILE"
	$DOIT chmod $4 ${DISCIMAGE}$2
	$DOIT chown $5 ${DISCIMAGE}$2

	echo "$3 `basename $1` -> $2"
	$DOIT echo "file $2" >>${DISCIMAGE}${DDK_INSTALL_LOG}
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

	$DOIT mkdir -p ${DESTDIR} || bail "Couldn't mkdir -p ${DESTDIR}"
	[ -n "$VERBOSE" ] && echo "Created directory `dirname $2`"

	# Delete the original so that permissions don't persist.
	#
	$DOIT rm -f $DESTFILE

	$DOIT ln -s $1 $DESTFILE || bail "Couldn't link $1 to $DESTFILE"
	$DOIT echo "link $2" >>${DISCIMAGE}${DDK_INSTALL_LOG}
	[ -n "$VERBOSE" ] && echo " linked `basename $1` -> $2"
}

# Tree-based installation function
# $1 = fromdir $2=destdir $3=blurb
#
install_tree()
{
	if [ ! -z $INSTALL_TARGET ]; then
		# Use rsync and SSH to do the copy as it is way faster.
		echo "rsyncing $3 to root@$INSTALL_TARGET:$2"
		$DOIT rsync -crlpt -e ssh $1/* root@$INSTALL_TARGET:$2 || bail "Couldn't rsync $1 to root@$INSTALL_TARGET:$2"
	else
		$DOIT mkdir -p ${DISCIMAGE}$2 || bail "Couldn't mkdir -p ${DISCIMAGE}$2"
		if [ -z "$DOIT" ]; then
			tar -C $1 -cf - . | tar -C ${DISCIMAGE}$2 -x${VERBOSE}f -
		else
			$DOIT "tar -C $1 -cf - . | tar -C ${DISCIMAGE}$2 -x${VERBOSE}f -"
		fi
	fi

	if [ $? = 0 ]; then
		echo "Installed $3 in ${DISCIMAGE}$2"
		$DOIT echo "tree $2" >>${DISCIMAGE}${DDK_INSTALL_LOG}
	else
		echo "Failed copying $3 from $1 to ${DISCIMAGE}$2"
	fi
}

# Uninstall something.
#
uninstall()
{
	if [ ! -f ${DISCIMAGE}${DDK_INSTALL_LOG} ]; then
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
				echo "No version record at head of ${DISCIMAGE}${DDK_INSTALL_LOG}"
			elif ! $DOIT rm -f ${DISCIMAGE}${data}; then
				BAD=1;
			else
				[ -n "$VERBOSE" ] && echo "Deleted $type $data"
			fi
			;;
		tree)
		  if [ "${data}" = "/usr/local/XSGX" ]; then
		    $DOIT rm -Rf ${DISCIMAGE}${data}
		  fi
			;;
		esac
	done < ${DISCIMAGE}${DDK_INSTALL_LOG};

	if [ $BAD = 0 ]; then
		echo "Uninstallation completed."
		$DOIT rm -f ${DISCIMAGE}${DDK_INSTALL_LOG}
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
	echo "         --no-pvr      don't install PowerVR driver components"
	echo "         --no-x        don't install X window system"
	echo "         --no-drm      don't install DRM libraries"
	echo "         --no-display  don't install integrated PowerVR display module"
	echo "         --no-bcdevice don't install buffer class device module"
	echo "         --root path   use path as the root of the install file system"
	exit 1
}

]]popdivert

###############################################################################
##
## Diversion 51 - the start of the install_pvr() function
##
###############################################################################
pushdivert(51)dnl
install_pvr()
{
	$DOIT echo "version PVRVERSION" >${DISCIMAGE}${DDK_INSTALL_LOG}
popdivert

pushdivert(53)dnl
}

popdivert

pushdivert(80)[[
# Work out if there are any special instructions.
#
while [ "$1" ]; do
	case "$1" in
	-v|--verbose)
		VERBOSE=v;
		;;
	-r|--root)
		DISCIMAGE=$2;
		shift;
		;;
	-t|--install-target)
		INSTALL_TARGET=$2;
		shift;
		;;
	-u|--uninstall)
		UNINSTALL=y
		;;
	-n)	DOIT=echo
		;;
	--no-pvr)
		NO_PVR=y
		;;
	--no-x)
		NO_X=y
		;;
	--no-drm)
		NO_DRM=y
		;;
	--no-display)
		NO_DISPLAYMOD=y
		;;
	--no-bcdevice)
		NO_BCDEVICE=y
		;;
	-h | --help | *)
		usage
		;;
	esac
	shift
done

# Find out where we are?  On the target?  On the host?
#
case `uname -m` in
arm*)	host=0;
		from=target;
		DISCIMAGE=/;
		;;
sh*)	host=0;
		from=target;
		DISCIMAGE=/;
		;;
i?86*)	host=1;
		from=host;
		if [ -z "$DISCIMAGE" ]; then
			echo "DISCIMAGE must be set for installation to be possible." >&2
			exit 1
		fi
		;;
x86_64*)	host=1;
		from=host;
		if [ -z "$DISCIMAGE" ]; then
			echo "DISCIMAGE must be set for installation to be possible." >&2
			exit 1
		fi
		;;
*)		echo "Don't know host to perform on machine type `uname -m`" >&2;
		exit 1
		;;
esac

if [ ! -z "$INSTALL_TARGET" ]; then
	if ssh -q -o "BatchMode=yes" root@$INSTALL_TARGET "test 1"; then
		echo "Using rsync/ssh to install to $INSTALL_TARGET"
	else
		echo "Can't access $INSTALL_TARGET via ssh."
		# We have to use the `whoami` trick as this script is often run with
		# sudo -E
		if [ ! -e ~`whoami`/.ssh/id_rsa.pub ] ; then
			echo " You need to generate a public key for root via ssh-keygen"
			echo " then append it to root@$INSTALL_TARGET:~/.ssh/authorized_keys."
		else
			echo "Have you installed root's public key into root@$INSTALL_TARGET:~/.ssh/authorized_keys?"
			echo "You can do so by executing the following as root:"
			echo "ssh root@$INSTALL_TARGET \"mkdir -p .ssh; cat >> .ssh/authorized_keys\" < ~/.ssh/id_rsa.pub"
		fi
		echo "Falling back to copy method."
		unset INSTALL_TARGET
	fi
fi

if [ ! -d "$DISCIMAGE" ]; then
	echo "$0: $DISCIMAGE does not exist." >&2
	exit 1
fi

echo
echo "Installing PowerVR Consumer/Embedded DDK ']]PVRVERSION[[' on $from"
echo
echo "File system installation root is $DISCIMAGE"
echo

]]popdivert


###############################################################################
##
## Main Diversion - call the install_pvr function unless NO_PVR is non-null
##
###############################################################################
pushmain()dnl
# Uninstall whatever's there already.
#
uninstall
[ -n "$UNINSTALL" ] && exit

#  Now start installing things we want.
#
[ -z "$NO_PVR" ] && install_pvr
popdivert


###############################################################################
##
## Diversion 200 -
## Divert some text to appear when the final input stream is closed.
##
###############################################################################
pushdivert(200)dnl

# All done...
#
echo
echo "Installation complete!"
if [ "$host" = 0 ]; then
   echo "You may now reboot your target."
fi
echo
popdivert
