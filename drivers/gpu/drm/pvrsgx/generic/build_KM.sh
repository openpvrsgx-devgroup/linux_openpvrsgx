#!/bin/bash
# Texas Instruments Incorporated

#-------------------------------------------------------------------------------
# Texas Instruments OMAP(TM) Platform Software
# (c) Copyright 2011 Texas Instruments Incorporated. All Rights Reserved.
# (c) Copyright 2002-2012 by Imagination Technologies Ltd. All Rights Reserved.
# Use of this software is controlled by the terms and conditions found
# in the license agreement under which this software has been supplied.
#-------------------------------------------------------------------------------


# Script to build the TI GFX Android DDK package (kernel modules)

function ECHO {
    echo -e $1
}

# Extract an absolute path from a directory passed as an argument
function getabspath() {
    _d=`dirname $1`
    readlink -f $_d
}

function help {

cat <<EOF

This script automatically builds and installs the DDK kernel modules.

GPU options:
    540          SGX540 (OMAP4430/OMAP4460)
    544sc        SGX544 single core (OMAP4470)
    544          SGX544 multi core (OMAP5)

Main options:
    -g <gpu>     Build kernel modules for <gpu> see GPU options
    --build      Build the kernel modules for the DDK
    --install    Install the kernel modules built by the DDK
    --binstall   Does both --build and --install
    --all        Does both --build and --install for available SGX cores
    --help       Show this help

<type> is one of the following:
    release      Build the kernel modules for the DDK in release mode
    debug        Build the kernel modules for the DDK in debug mode
    timing       Build the kernel modules for the DDK in timing mode

Optional build arguments (only one at a time):
    clean        Clean built object files
    clobber      Remove built binaries (doesn't clean up install)
    V=1          Increase verbosity level at build time

The following variables are mandatory:
    KERNELDIR    Source location of kernel
    DISCIMAGE    Installation location for the DDK binaries.

EOF
}

function usage {
    ECHO "Usage: ./build_KM.sh [-g 540|544sc|544] --build <type> [clean | clobber]"
    ECHO "Usage: ./build_KM.sh [-g 540|544sc|544] --install <BUILD_TYPE>"
    ECHO "Usage: ./build_KM.sh [-g 540|544sc|544] --binstall <BUILD_TYPE>"
    ECHO "Usage: ./build_KM.sh --all <BUILD_TYPE>"
    ECHO "Usage: ./build_KM.sh --help"
}

# needs ARG_OPERATION
# sets ARG_PRODUCT
#function get_args_androidbuild {
#    if [ "$ARG_OPERATION" = "--build" ] || \
#       [ "$ARG_OPERATION" = "--binstall" ] || \
#       [ "$ARG_OPERATION" = "--install" ] ; then
#        if [ -z "$ANDROID_PRODUCT_OUT" ] || \
#           [ -z "$OUT" ] || \
#           [ -z "$ANDROID_BUILD_TOP" ]; then
#            ECHO "ERROR: Initialize the Android build environment"
#            ECHO "cd \"YOUR_ANDROID_SRC\" ; . build/envsetup.sh; lunch"
#            exit;
#     else
#            ARG_PRODUCT=`basename $ANDROID_PRODUCT_OUT`
#        fi
#    fi
#}

# needs ARG_BUILD_TYPE
function get_args_buildtype {
    # Validate build / install modes
    if [[ -n "$ARG_BUILD_TYPE" ]] ; then
        declare -A mode_types=([debug]="1" [release]="1" [timing]="1")
        if [[ -z "${mode_types[$ARG_BUILD_TYPE]}" ]] ; then
            ECHO "ERROR: Unknown build type: $ARG_BUILD_TYPE"
            exit
        fi
    fi
}

# Set up global args (ARG_name) depending on operation to be performed
function args_getenv {
#    ARG_GPU=540
    if [ -z "$DISCIMAGE" ] ; then
        ECHO "ERROR: Need to set path to installation directory, i.e.:"
        ECHO "export DISCIMAGE=<install location>"
        exit 1
    elif [ -z "$KERNELDIR" ] ; then
        ECHO "ERROR: Need to set path to compiled kernel directory, i.e.:"
        ECHO "export KERNELDIR=<compiled kernel location>"
        exit 1
    fi
    while [ $# -ne 0 ] ; do
        if [ "$1" = "--build" ] || \
           [ "$1" = "--binstall" ] || \
           [ "$1" = "--all" ] || \
           [ "$1" = "--install" ] ; then
            if (( $# < 2)) ; then
                ECHO "ERROR: Too few args"; usage; exit;
            fi
            ARG_OPERATION=$1
 #           get_args_androidbuild
            ARG_BUILD_TYPE=$2
            get_args_buildtype
            ARG_OPTIONAL=${@:3}
            if [ -z "$ARG_GPU" ] && [ "$1" = "--all" ] ; then
                break;
            else
                 ECHO "ERROR: Please select a GPU type"; usage; exit;
            fi

#        elif [ "$1" = "--bench" ] ; then
#            ARG_BMARK=$2
#            if [ -z "$DISCIMAGE" ] ; then
#                ECHO "ERROR: Need to set path to installation directory, i.e.:"
#                ECHO "export DISCIMAGE=<install location>"
#                exit 1
#            fi
#            ARG_OPERATION=$1
#            if (( $# < 2 )) ; then
#                ECHO "ERROR: Too few args"; usage; exit;
#            fi
#            break;
        elif [ "$1" = "--help" ] ; then
            help
            usage
            exit
        elif [ "$1" = "-g" ] ; then
            shift;
            ARG_GPU=$1
            case "$ARG_GPU" in
            540)
                ;;
            544sc)
                ;;
            544)
                ;;
            *)
                ECHO "ERROR: Unknown GPU type"; usage; exit;
            esac
        else
            ECHO "ERROR: Unknown arg"
            usage
            exit
        fi
        shift
    done

    if (( $# < 1)) ; then
        ECHO "ERROR: Too few args"; usage; exit;
    fi


    if [ "$ARG_GPU" = "540" ] ; then
        ARG_BUILDPLATDIR=omap4430_android
        ARG_BINLOC_PREFIX=binary2_540_120
        ARG_KM_SUFFIX=_sgx540_120
        ARG_PRODUCT=blaze_tablet
    elif [ "$ARG_GPU" = "544sc" ] ; then
        ARG_BUILDPLATDIR=omap4430_android
        ARG_BINLOC_PREFIX=binary2_544_112
        ARG_KM_SUFFIX=_sgx544_112
        ARG_PRODUCT=blaze_tablet
    elif [ "$ARG_GPU" = "544" ] ; then
        ARG_BUILDPLATDIR=omap5430_android
        ARG_BINLOC_PREFIX=binary2_544_tbd
        ARG_KM_SUFFIX=_sgx544_105
        ARG_PRODUCT=blaze_tablet
    fi
}

# Needs ANDROID_BUILD_TOP, G_GFXDDKDIR, ARG_BUILDPLATDIR
# Sets BUILD_DIR
function build_getenv {

    # Variables to update by this function
#    BUILD_DIR=
    BUILD_DIR_KM=

    # Assume args_getenv validated we are in an Android build env
#    export ANDROID_ROOT="$ANDROID_BUILD_TOP"

    # We know at this point we have a valid ARG_PRODUCT
#    BUILD_DIR=${G_GFXDDKDIR}/eurasia/eurasiacon/build/linux2/$ARG_BUILDPLATDIR
    BUILD_DIR_KM=${G_GFXDDKDIR}/eurasia_km/eurasiacon/build/linux2/$ARG_BUILDPLATDIR
}

# Needs G_GFXDDKDIR, ARG_BUILDPLATDIR
# Sets INSTALL_DIR
function install_getenv {
    if [ -z "$DISCIMAGE" ] ; then
        export DISCIMAGE="$OUT"
    fi
    INSTALL_DIR=${G_GFXDDKDIR}/eurasia/eurasiacon/build/linux2/$ARG_BUILDPLATDIR
}

function do_build {
    BUILD_DIR=
    build_getenv

#    ECHO "-- Building PVR-UM for SGX$ARG_GPU--"

#    export CROSS_COMPILE=arm-eabi-

    # The DDK makefiles will guess the value of EURASIAROOT on the basis
    # of the location of the build directory we cd to
#    pushd $BUILD_DIR
    # We could leave OUT as set and the DDK makefiles will install binaries
    # on the basis of this location. For now rely on the install step to
    # put stuff in the right place in the FS
#    local _OUT=$OUT
#    unset OUT
#    make TARGET_PRODUCT="$ARG_PRODUCT" BUILD=$ARG_BUILD_TYPE TARGET_SGX=$ARG_GPU $ARG_OPTIONAL
#    OUT=$_OUT
#    popd

    ECHO "-- Building PVR-KM for SGX$ARG_GPU--"

    pushd $BUILD_DIR_KM
    local _OUT=$OUT
    unset OUT
    make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- TARGET_PRODUCT="$ARG_PRODUCT" BUILD=$ARG_BUILD_TYPE TARGET_SGX=$ARG_GPU $ARG_OPTIONAL
    OUT=$_OUT
    popd



}

function do_install {
    install_getenv

#    ECHO "-- Installing PVR-UM to $DISCIMAGE --"

#    pushd $INSTALL_DIR
#    local _OUT=$OUT
#    unset OUT
#    make TARGET_PRODUCT="$ARG_PRODUCT" BUILD=$ARG_BUILD_TYPE TARGET_SGX=$ARG_GPU install
#    OUT=$_OUT
#    popd

    ECHO "-- Installing PVR-KM to $DISCIMAGE --"

    _TARGET_KM=${G_GFXDDKDIR}/eurasia_km/eurasiacon/${ARG_BINLOC_PREFIX}_${ARG_BUILDPLATDIR}_${ARG_BUILD_TYPE}/target

    for _m in omaplfb${ARG_KM_SUFFIX}.ko pvrsrvkm${ARG_KM_SUFFIX}.ko
    do
        _MOD_LOC="/system/lib/modules"
        if [ ! -d "${DISCIMAGE}${_MOD_LOC}" ] ; then
            mkdir -p "${DISCIMAGE}${_MOD_LOC}"
        fi
        _cp_cmd="cp ${_TARGET_KM}/${_m} ${DISCIMAGE}${_MOD_LOC}"
        echo "${_m} -> ${_MOD_LOC}/${_m}"
        eval "$_cp_cmd"
    done
    ECHO "\nInstallation complete!"
}


# ##############################################################################
# MAIN - This is where the script starts executing
# ##############################################################################

# We will assume that this script is invoked from the top of the DDK
G_GFXDDKDIR=`getabspath $0`

#  Get ARG_OPERATION, ARG_* parameters appropriate for the operation
args_getenv $@

case "$ARG_OPERATION" in
# ##############################################################################
    --build)
    do_build
    ;;

# ##############################################################################
    --install)
    do_install
    ;;

# ##############################################################################
    --binstall)
    do_build
    do_install
    ;;

# ##############################################################################
    --all)
        ARG_GPU=540
        ARG_BUILDPLATDIR=omap4430_android
        ARG_BINLOC_PREFIX=binary2_540_120
        ARG_KM_SUFFIX=_sgx540_120
        ARG_PRODUCT=blaze_tablet
    do_build
    do_install
        ARG_GPU=544sc
        ARG_BUILDPLATDIR=omap4430_android
        ARG_BINLOC_PREFIX=binary2_544_112
        ARG_KM_SUFFIX=_sgx544_112
        ARG_PRODUCT=blaze_tablet
    do_build
    do_install
    ;;

# ##############################################################################
#    --bench)

#    declare -A inst_src
#    inst_src[GridMark3]="${G_GFXDDKDIR}/Perfapps/OGLES*/GridMark3/AndroidARMV7/ReleaseRaw/*"
#    inst_src[VillageMark]="${G_GFXDDKDIR}/Perfapps/OGLES*/VillageMark/AndroidARMV7/ReleaseRaw/*"
#    inst_src[TMark]="${G_GFXDDKDIR}/Perfapps/OGLES*/TMark/AndroidARMV7/ReleaseRaw/*"
#    inst_src[FableMark]="${G_GFXDDKDIR}/Perfapps/OGLES2/FableMark/AndroidARMV7/ReleaseRaw/*"
#    inst_src[UIMark]="${G_GFXDDKDIR}/Perfapps/MISC/UIMark/PVR2D_SGX/*"

#    if [[ $ARG_BMARK != "all" && -z "${inst_src[$ARG_BMARK]}" ]] ; then
#        ECHO "ERROR: Unknown benchmark specified"
#        exit 1
#    fi

#    ECHO "= Installing $ARG_BMARK benchmarks ="
#    if [ $ARG_BMARK = "all" ] ; then
#        _failcp=0
#        for k in "${!inst_src[@]}"
#        do
#            _cp_cmd="cp ${inst_src[$k]} ${DISCIMAGE}/system/vendor/bin"
#            echo DOING "$_cp_cmd"
#            if [[ $_failcp = "0" ]] ; then
#                eval "$_cp_cmd"
#                [[ $? != 0 ]] && break
#            fi
#        done

#    else
#        _cp_cmd="cp ${inst_src[$ARG_BMARK]} ${DISCIMAGE}/system/vendor/bin"
#        echo DOING "$_cp_cmd"
#        eval "$_cp_cmd"
#    fi
#    ;;
#    *)
#    usage
#    ;;
# ##############################################################################
esac


