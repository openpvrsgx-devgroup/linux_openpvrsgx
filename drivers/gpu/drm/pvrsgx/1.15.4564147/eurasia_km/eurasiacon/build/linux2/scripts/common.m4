divert(-1)
########################################################################### ###
##@Title         Target common m4 macros.
##@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
##@License       Dual MIT/GPLv2
## 
## The contents of this file are subject to the MIT license as set out below.
## 
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
## 
## The above copyright notice and this permission notice shall be included in
## all copies or substantial portions of the Software.
## 
## Alternatively, the contents of this file may be used under the terms of
## the GNU General Public License Version 2 ("GPL") in which case the provisions
## of GPL are applicable instead of those above.
## 
## If you wish to allow use of your version of this file only under the terms of
## GPL, and not to allow others to use your version of this file under the terms
## of the MIT license, indicate your decision by deleting the provisions above
## and replace them with the notice and other provisions required by GPL as set
## out in the file called "GPL-COPYING" included in this distribution. If you do
## not delete the provisions above, a recipient may use your version of this file
## under the terms of either the MIT license or GPL.
## 
## This License is also included in this distribution in the file called
## "MIT-COPYING".
## 
## EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
## PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
## BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
## PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
## COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
## IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
## CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
### ###########################################################################

# Define how we quote things.
#
changequote([[, ]])

# Define how we comment things.  We don't define a comment end delimiter here so
# the end-of-line serves that function.  Why change the comment starter?  We do
# this so we that we can have macro replacement in text intended as comments in
# the output stream.  The default starter is '#'; "dnl" is also unusable.  Note
# that the way we've set up the diversion discipline means that only comments
# inside pushdivert...popdivert pairs will be copied anyway.
#
changecom([[##]])

#! Some macros to handle diversions conveniently.
#!
define([[_current_divert]],	[[-1]])
define([[pushdivert]],		[[pushdef([[_current_divert]],$1)divert($1)]])
define([[popdivert]],		[[popdef([[_current_divert]])divert(_current_divert)]])
define([[pushmain]],		[[pushdivert(100)]])

## Diversion discipline
##
## 0		standard-output stream - DO NOT USE
## 1		shell script interpreter directive
## 2		copyright
## 3		version trace
## 4-49	for use only by common.m4
## 50-99	for use by other macro bodies (rc.pvr.m4, install.sh.m4 etc.)
## 100	main output body of script.
## 200	macro bodies's trailers
## 201	common.m4's trailers
##

###############################################################################
##
##	Diversion #1 - Copy in the interpreter line
##
###############################################################################
define([[INTERPRETER]], [[pushdivert(1)dnl
#!/bin/sh
popdivert]])


###############################################################################
##
##	Diversion #2 - Copy in the copyright text.
##  NOTE: The following license applies to the generated files, not this file.
###############################################################################
pushdivert(2)dnl
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       MIT
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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

popdivert


###############################################################################
##
##	Some defines for useful constants
##
###############################################################################
define([[TRUE]],					[[1]])
define([[FALSE]],					[[0]])


###############################################################################
##
##	Some defines for where things go.
##
###############################################################################
define([[RC_DESTDIR]],				[[/etc/init.d]])
define([[MODULES_DEP]],				[[${DISCIMAGE}MOD_ROOTDIR/modules.dep]])
define([[MODULES_TMP]],				[[/tmp/modules.$$.tmp]])
define([[MODULES_CONF]],			[[${DISCIMAGE}/etc/modules.conf]])
define([[MODPROBE_CONF]],			[[${DISCIMAGE}/etc/modprobe.conf]])
define([[LIBMODPROBE_CONF]],		[[${DISCIMAGE}/lib/modules/modprobe.conf]])
define([[PATH_DEPMOD]],				[[/sbin/depmod]])
define([[PATH_MODPROBE]],			[[/sbin/modprobe]])
define([[APK_DESTDIR]],				[[/data/app]])
define([[HAL_DESTDIR]],				[[SHLIB_DESTDIR/hw]])

###############################################################################
##
## Diversion #3 - start of the versioning trace information.
##
###############################################################################
pushdivert(3)dnl
[[#]] Auto-generated for PVR_BUILD_DIR from ifelse(BUILD,release, build: PVRVERSION)
popdivert


###############################################################################
##
## Diversion #4 - end of versioning trace information.
## Leave a comment line and a blank line for output tidiness.
##
###############################################################################
pushdivert(4)dnl
#

popdivert


## The following two commented line are templates for creating 'variables' which
## operate thus:
##
## A(5) -> A=5
## A -> outputs A
## CAT_A(N) -> A=A,N
##
## define([[A]], [[ifelse($#, 0, _A, [[define([[_A]], $1)]])]])dnl
## define([[CAT_A]], [[define([[_A]], ifelse(defn([[_A]]),,$1,[[defn([[_A]])[[,$1]]]]))]])


###############################################################################
##
## TARGET_HAS_DEPMOD -
## Indicates that the target has a depmod binary available.  This means we
## don't have to update modules.dep manually - unpleasant.
##
###############################################################################
define([[TARGET_HAS_DEPMOD]], [[ifelse($#, 0, _$0, [[define([[_$0]], $1)]])]])


###############################################################################
##
## TARGET_RUNS_DEPMOD_AT_BOOT
## Indicates that the target runs depmod every boot time.  This means we don't
## have to run it ourselves.
##
###############################################################################
define([[TARGET_RUNS_DEPMOD_AT_BOOT]], [[ifelse($#, 0, _$0, [[define([[_$0]], $1)]])]])

define([[DISPLAY_CONTROLLER]],		[[define([[_DISPLAY_CONTROLLER_]], $1),
									  define([[_DISPLAY_CONTROLLER_FAILED_]], $1_failed)]])

define([[BUFFER_CLASS_DEVICE]],		[[define([[_BUFFER_CLASS_DEVICE_]], $1),
									  define([[_BUFFER_CLASS_DEVICE_FAILED_]], $1_failed)]])


###############################################################################
##
## INSTALL_KERNEL_MODULE -
## install a kernel loadable module in the correct place in the file system,
## updating the module dependencies file(s) if necessary.
##
## Parameters:
## $1 - name of kernel module file without leading path components
## $2 - name of any other kernel module upon which $1 is dependent.
##
###############################################################################
define([[INSTALL_KERNEL_MODULE]], [[pushdivert(52)dnl
	install_file $1 MOD_DESTDIR/$1 "kernel module" 0644 0:0
ifelse(TARGET_RUNS_DEPMOD_AT_BOOT,TRUE,,TARGET_HAS_DEPMOD,FALSE,[[dnl
FIXUP_MODULES_DEP($1, $2)dnl
]],[[dnl
	if [ "$host" = 1 ]; then
		FIXUP_MODULES_DEP($1, $2)dnl
	fi
]])dnl
popdivert()]])


###############################################################################
##
## FIXUP_MODULES_DEP -
## Fix up the modules dependency file.
##
## Parameters:
## $1 - name of kernel module file without leading path components
## $2 - name of any other kernel module upon which $1 is dependent.
##
###############################################################################
define([[FIXUP_MODULES_DEP]], [[pushdivert(52)dnl
	grep -v -e "extra/$1" MODULES_DEP >MODULES_TMP
ifelse([[$2]], [[]],dnl
	echo "MOD_DESTDIR/$1:" >>MODULES_TMP,
	echo "MOD_DESTDIR/$1: MOD_DESTDIR/$2" >>MODULES_TMP)
	cp MODULES_TMP MODULES_DEP
popdivert()]])


###############################################################################
##
## INSTALL_SHARED_LIBRARY -
## Install a shared library (in /usr/lib) with the correct version number and
## links.
##
## Parameters:
## $1 - name of shared library to install
## $2 - optional version suffix.
##
###############################################################################
define([[INSTALL_SHARED_LIBRARY]], [[pushdivert(52)dnl
	ifelse($2,,install_file $1 SHLIB_DESTDIR/$1 "shared library" 0644 0:0,install_file $1 SHLIB_DESTDIR/$1.$2 "shared library" 0644 0:0
	install_link $1.$2 SHLIB_DESTDIR/$1)
popdivert()]])


###############################################################################
##
## INSTALL_SHARED_LIBRARY_DESTDIR -
## Install a shared library in a non-standard library directory, with the
## correct version number and links.
##
## Parameters:
## $1 - name of shared library to install
## $2 - optional version suffix
## $3 - name of a subdirectory to install to
##
###############################################################################
define([[INSTALL_SHARED_LIBRARY_DESTDIR]], [[pushdivert(52)dnl
	ifelse($2,,install_file $1 $3/$1 "shared library" 0644 0:0,install_file $1 $3/$1.$2 "shared library" 0644 0:0
	install_link $1.$2 $3/$1)
popdivert()]])


###############################################################################
##
## INSTALL_BINARY -
## Install a binary file.  These always go in /usr/local/bin presently.
##
## Parameters:
## $1 - name of file in local directory which is to be copied to /usr/local/bin
##
###############################################################################
define([[INSTALL_BINARY]], [[pushdivert(52)dnl
	install_file $1 DEMO_DESTDIR/$1 "binary" 0755 0:0
popdivert()]])

###############################################################################
##
## INSTALL_APK -
## Install an .apk file. These are only installed on Android.
##
## Parameters:
## $1 - name of file in local directory which is to be copied to /data/app
##
###############################################################################
define([[INSTALL_APK]], [[pushdivert(52)dnl
	install_file $1.apk APK_DESTDIR/$1.apk "binary" 0644 1000:1000
popdivert()]])

###############################################################################
##
## INSTALL_SHADER -
## Install a binary file.  These always go in /usr/local/bin presently.
##
## Parameters:
## $1 - name of file in local directory which is to be copied to /usr/local/bin
##
###############################################################################
define([[INSTALL_SHADER]], [[pushdivert(52)dnl
	install_file $1 DEMO_DESTDIR/$1 "shader" 0644 0:0
popdivert()]])


###############################################################################
##
## INSTALL_INITRC -
## Install a run-time configuration file.  This goes in /etc/init.d unless
## RC_DESTDIR has been updated.
##
## No parameters.
##
###############################################################################
define([[INSTALL_INITRC]], [[pushdivert(52)dnl
	install_file $1 RC_DESTDIR/$1 "boot script" 0755 0:0
popdivert()]])

###############################################################################
##
## INSTALL_HEADER -
## Install a header. These go in HEADER_DESTDIR, which is normally
## /usr/include.
##
## Parameters:
## $1 - name of header to install
## $2 - subdirectory of /usr/include to install it to
##
###############################################################################
define([[INSTALL_HEADER]], [[pushdivert(52)dnl
	install_file $1 HEADER_DESTDIR/$2/$1 "header" 0644 0:0
popdivert()]])

###############################################################################
##
## Some defines for general expansion
##
## No parameters.
##
###############################################################################
define([[SHARED_LIBRARY]],			[[INSTALL_SHARED_LIBRARY($1,$2)]])
define([[SHARED_LIBRARY_DESTDIR]],	[[INSTALL_SHARED_LIBRARY_DESTDIR($1, $2, $3)]])
define([[APK]],						[[INSTALL_APK($1)]])
define([[UNITTEST]],				[[INSTALL_BINARY($1)]])
define([[EXECUTABLE]],				[[INSTALL_BINARY($1)]])
define([[GLES1UNITTEST]],			[[INSTALL_BINARY($1)]])
define([[GLES2UNITTEST]],			[[INSTALL_BINARY($1)]])
define([[GLES2UNITTEST_SHADER]],	[[INSTALL_SHADER($1)]])
define([[GLUNITTEST_SHADER]],		[[INSTALL_SHADER($1)]])
define([[HEADER]],					[[INSTALL_HEADER($1, $2)]])
define([[KERNEL_MODULE]],			[[INSTALL_KERNEL_MODULE($1, $2)]])
# FIXME:Services 4.0:
# These should be installed somewhere else
define([[INITBINARY]],				[[INSTALL_BINARY($1)]])


###############################################################################
##
## STANDARD_SCRIPTS -
## Install all standard script parts of a DDK distribution.
##
## No parameters.
##
###############################################################################
define([[STANDARD_SCRIPTS]], [[pushdivert(52)dnl
	# Install the standard scripts
	#
INSTALL_INITRC(rc.pvr)dnl

popdivert()]])


###############################################################################
##
## STANDARD_LIBRARIES
## Install all standard parts of a DDK distribution.
##
## No parameters.
##
###############################################################################
define([[STANDARD_LIBRARIES]], [[pushdivert(52)dnl
	# Install the standard libraries
	#
ifelse(SUPPORT_OPENGLES1_V1_ONLY,0,
ifelse(SUPPORT_OPENGLES1,1, [[SHARED_LIBRARY_DESTDIR([[OGLES1_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]]))dnl

ifelse(SUPPORT_OPENGLES1_V1,1,[[SHARED_LIBRARY_DESTDIR([[OGLES1_V1_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]])dnl
ifelse(SUPPORT_OPENGLES1_V1_ONLY,1,[[SHARED_LIBRARY_DESTDIR([[OGLES1_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]])dnl

ifelse(FFGEN_UNIFLEX,1,[[SHARED_LIBRARY([[libusc.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_OPENGLES2,1,[[SHARED_LIBRARY_DESTDIR([[OGLES2_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]])dnl

ifelse(SUPPORT_OPENGLES2,1,
ifelse(SUPPORT_SOURCE_SHADER,1,[[SHARED_LIBRARY([[libglslcompiler.LIB_SUFFIX]],[[SOLIB_VERSION]])]]))dnl

ifelse(SUPPORT_OPENCL,1,[[SHARED_LIBRARY([[libPVROCL.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_OPENCL,1,[[SHARED_LIBRARY([[liboclcompiler.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_OPENCL,1,
	ifelse(SUPPORT_ANDROID_PLATFORM,1,[[SHARED_LIBRARY([[libocl1test1.so]],[[SOLIB_VERSION]])]]))dnl

ifelse(SUPPORT_RSCOMPUTE,1,[[SHARED_LIBRARY([[libPVRRS.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_RSCOMPUTE,1,[[SHARED_LIBRARY([[librsccompiler.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_RSCOMPUTE,1,[[SHARED_LIBRARY([[librsccore.bc]],[[SOLIB_VERSION]])]])dnl

SHARED_LIBRARY([[libIMGegl.LIB_SUFFIX]],[[SOLIB_VERSION]])dnl
ifelse(SUPPORT_LIBEGL,1,[[SHARED_LIBRARY_DESTDIR([[EGL_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]])dnl
ifelse(SUPPORT_LIBPVR2D,1,[[SHARED_LIBRARY([[libpvr2d.LIB_SUFFIX]],[[SOLIB_VERSION]])]],
						  [[SHARED_LIBRARY([[libnullws.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_NULL_PVR2D_BLIT,1,[[SHARED_LIBRARY([[libpvrPVR2D_BLITWSEGL.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_NULL_PVR2D_FLIP,1,[[SHARED_LIBRARY([[libpvrPVR2D_FLIPWSEGL.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_NULL_PVR2D_FRONT,1,[[SHARED_LIBRARY([[libpvrPVR2D_FRONTWSEGL.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_NULL_PVR2D_LINUXFB,1,[[SHARED_LIBRARY([[libpvrPVR2D_LINUXFBWSEGL.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_NULL_DRM_WS,1,[[SHARED_LIBRARY([[libpvrDRMWSEGL.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_EWS,1,[[SHARED_LIBRARY([[libpvrEWS_WSEGL.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

SHARED_LIBRARY([[libdbm.LIB_SUFFIX]],[[SOLIB_VERSION]])dnl

SHARED_LIBRARY([[libsrv_um.LIB_SUFFIX]],[[SOLIB_VERSION]])dnl
ifelse(SUPPORT_SRVINIT,1,[[SHARED_LIBRARY([[libsrv_init.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_SGX_HWPERF,1,[[SHARED_LIBRARY([[libPVRScopeServices.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_IMGTCL,1,[[SHARED_LIBRARY([[libimgtcl.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_OPENGL,1,[[SHARED_LIBRARY([[libPVROGL.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_XORG,1,,[[
ifelse(SUPPORT_OPENGL,1,  install_link libPVROGL.LIB_SUFFIX.SOLIB_VERSION SHLIB_DESTDIR/libGL.LIB_SUFFIX.1
  install_link libPVROGL.LIB_SUFFIX.SOLIB_VERSION SHLIB_DESTDIR/libGL.LIB_SUFFIX)]])

ifelse(SUPPORT_MESA,1,[[SHARED_LIBRARY([[libpvr_dri_support.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY_DESTDIR([[GRALLOC_MODULE]],[[SOLIB_VERSION]],[[HAL_DESTDIR]])]])dnl
ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY_DESTDIR([[HWCOMPOSER_MODULE]],[[SOLIB_VERSION]],[[HAL_DESTDIR]])]])dnl
ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY_DESTDIR([[CAMERA_MODULE]],[[SOLIB_VERSION]],[[HAL_DESTDIR]])]])dnl
ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY_DESTDIR([[SENSORS_MODULE]],[[SOLIB_VERSION]],[[HAL_DESTDIR]])]])dnl
ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY_DESTDIR([[MEMTRACK_MODULE]],[[SOLIB_VERSION]],[[HAL_DESTDIR]])]])dnl
ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY([[libpvrANDROID_WSEGL.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY([[libcreatesurface.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_EWS,1,[[SHARED_LIBRARY([[libews.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

popdivert()]])



###############################################################################
##
## STANDARD_EXECUTABLES
## Install all standard executables of a DDK distribution.
##
## No parameters.
##
###############################################################################
define([[STANDARD_EXECUTABLES]], [[pushdivert(52)dnl
	# Install the standard executables
	#

ifelse(SUPPORT_SRVINIT,1,[[INITBINARY(pvrsrvctl)]])dnl
ifelse(SUPPORT_SRVINIT,1,[[UNITTEST(sgx_init_test)]])dnl

ifelse(PVR_REMVIEW,1,[[EXECUTABLE(pvrremview)]])dnl
ifelse(PDUMP,1,[[UNITTEST(pdump)]])dnl

ifelse(SUPPORT_EWS,1,[[EXECUTABLE(ews_server)]])dnl
ifelse(SUPPORT_EWS,1,
	ifelse(SUPPORT_OPENGLES2,1,[[EXECUTABLE(ews_server_es2)]]))dnl
ifelse(SUPPORT_EWS,1,
	ifelse(SUPPORT_LUA,1,[[EXECUTABLE(ews_wm)]]))dnl

popdivert()]])
 
###############################################################################
##
## STANDARD_UNITTESTS
## Install all standard unitests built as part of a DDK distribution.
##
## No parameters.
##
###############################################################################
define([[STANDARD_UNITTESTS]], [[pushdivert(52)dnl
	# Install the standard unittests
	#

ifelse(SUPPORT_ANDROID_PLATFORM,1,[[
	if [ ! -d ${DISCIMAGE}/data ]; then
		mkdir ${DISCIMAGE}/data
		chown 1000:1000 ${DISCIMAGE}/data
		chmod 0771 ${DISCIMAGE}/data
	fi

	if [ ! -d ${DISCIMAGE}/data/app ]; then
		mkdir ${DISCIMAGE}/data/app
		chown 1000:1000 ${DISCIMAGE}/data/app
		chmod 0771 ${DISCIMAGE}/data/app
	fi
]])dnl

UNITTEST(services_test)dnl
UNITTEST(sgx_blit_test)dnl
UNITTEST(sgx_clipblit_test)dnl
UNITTEST(sgx_flip_test)dnl
UNITTEST(sgx_render_flip_test)dnl
UNITTEST(pvr2d_test)dnl

ifelse(SUPPORT_ANDROID_PLATFORM,1,[[
UNITTEST(testwrap)dnl
ifelse(SUPPORT_OPENGLES1,1,[[
APK(gles1test1)dnl
APK(gles1_texture_stream)dnl
]])dnl
ifelse(SUPPORT_OPENGLES2,1,[[
APK(gles2test1)dnl
APK(gles2_texture_stream)dnl
]])dnl
ifelse(SUPPORT_OPENCL,1,[[
APK(ocl1test1)dnl
]])dnl
APK(eglinfo)dnl
APK(launcher)dnl
ifelse(SUPPORT_RSCOMPUTE,1,[[
UNITTEST(rsc_unit_test)dnl
UNITTEST(rsc_unit_test2)dnl
UNITTEST(rsc_runtime_test)dnl
UNITTEST(rsc_runtime_test2)dnl
]])dnl
]],[[
ifelse(SUPPORT_OPENGLES1,1,[[
GLES1UNITTEST(gles1test1)dnl
GLES1UNITTEST(gles1_texture_stream)dnl
]])dnl
ifelse(SUPPORT_OPENGLES2,1,[[
GLES2UNITTEST(gles2test1)dnl
GLES2UNITTEST_SHADER(glsltest1_vertshader.txt)dnl
GLES2UNITTEST_SHADER(glsltest1_fragshaderA.txt)dnl
GLES2UNITTEST_SHADER(glsltest1_fragshaderB.txt)dnl
GLES2UNITTEST(gles2_texture_stream)dnl
]])dnl
ifelse(SUPPORT_OPENGL,1,[[
UNITTEST(gltest1)dnl
UNITTEST(gltest2)dnl
GLUNITTEST_SHADER(gltest2_vertshader.txt)dnl
GLUNITTEST_SHADER(gltest2_fragshaderA.txt)dnl
GLUNITTEST_SHADER(gltest2_fragshaderB.txt)dnl
]])dnl
UNITTEST(eglinfo)dnl
ifelse(SUPPORT_OPENCL,1,[[
UNITTEST(ocl_unit_test)dnl
UNITTEST(ocl_filter_test)dnl
]])dnl
]])dnl

ifelse(SUPPORT_ANDROID_PLATFORM,1,[[
UNITTEST(framebuffer_test)dnl
UNITTEST(texture_benchmark)dnl
]])dnl

ifelse(SUPPORT_XUNITTESTS,1,[[
UNITTEST(xtest)dnl
UNITTEST(xeglinfo)dnl
ifelse(SUPPORT_OPENGLES1,1,[[
GLES1UNITTEST(xgles1test1)dnl
GLES1UNITTEST(xmultiegltest)dnl
GLES1UNITTEST(xgles1_texture_stream)dnl
GLES2UNITTEST(xgles1image)dnl
GLES1UNITTEST(xgles1image_external)dnl
]])dnl
ifelse(SUPPORT_OPENGLES2,1,[[
GLES2UNITTEST(xgles2test1)dnl
GLES2UNITTEST(xgles2_texture_stream)dnl
GLES2UNITTEST(xgles2image)dnl
GLES2UNITTEST_SHADER(xgles2image_vertshader.txt)dnl
GLES2UNITTEST_SHADER(xgles2image_fragshaderA.txt)dnl
GLES2UNITTEST_SHADER(xgles2image_fragshaderB.txt)dnl
GLES2UNITTEST_SHADER(xgles2image_vertshaderW.txt)dnl
GLES2UNITTEST_SHADER(xgles2image_fragshaderW.txt)dnl
GLES2UNITTEST(xgles2image_external)dnl
GLES2UNITTEST_SHADER(xgles2image_external_vertshader.txt)dnl
GLES2UNITTEST_SHADER(xgles2image_external_fragshader.txt)dnl
]])dnl
ifelse(SUPPORT_OPENGL,1,[[
UNITTEST(xgltest1)dnl
UNITTEST(xgltest2)dnl
GLUNITTEST_SHADER(gltest2_vertshader.txt)dnl
GLUNITTEST_SHADER(gltest2_fragshaderA.txt)dnl
GLUNITTEST_SHADER(gltest2_fragshaderB.txt)dnl
]])dnl
]])dnl

ifelse(SUPPORT_EWS,1,[[
ifelse(SUPPORT_OPENGLES1,1,[[
GLES1UNITTEST(ews_test_gles1)dnl
GLES2UNITTEST(ews_test_gles1_egl_image_external)dnl
]])dnl
ifelse(SUPPORT_OPENGLES2,1,[[
GLES2UNITTEST(ews_test_gles2)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_main.vert)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_main.frag)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_pp.vert)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_pp.frag)dnl
GLES2UNITTEST(ews_test_gles2_egl_image_external)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_egl_image_external.vert)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_egl_image_external.frag)dnl
]])dnl
UNITTEST(ews_test_swrender)dnl
]])dnl

ifelse(SUPPORT_WAYLAND,1,[[
  ifelse(SUPPORT_OPENGLES1,1,[[
    GLES1UNITTEST(wgles1test1)dnl
  ]])dnl
  ifelse(SUPPORT_OPENGLES2,1,[[
    GLES2UNITTEST(wgles2test1)dnl
  ]])dnl
]])dnl

ifelse(SUPPORT_SURFACELESS,1,[[
  ifelse(SUPPORT_OPENGLES2,1,[[
    GLES2UNITTEST(sgles2test1)dnl
  ]])dnl
]])dnl

popdivert()]])


define([[NON_SHIPPING_TESTS]], [[pushdivert(52)dnl
	# Install internal unittests
	#
	mkdir -p $DISCIMAGE/usr/local/bin/gles2tests
	if test -d $EURASIAROOT/eurasiacon/opengles2/tests; then
		for test in $(find $EURASIAROOT/eurasiacon/opengles2/tests -regex '.*makefile.linux' -exec cat {} \;|egrep 'MODULE\W*='|dos2unix|cut -d '=' -f2)
		do
			if test -f $test; then
				install_file $test /usr/local/bin/gles2tests/$test "binary" 0755 0:0
			fi
		done

		echo "Shaders -> /usr/local/bin/glestests"
		find $EURASIAROOT/eurasiacon/opengles2/tests -regex '.*\.vert' -exec cp -f {} $DISCIMAGE/usr/local/bin/gles2tests \;
		find $EURASIAROOT/eurasiacon/opengles2/tests -regex '.*\.frag' -exec cp -f {} $DISCIMAGE/usr/local/bin/gles2tests \;
	fi
	
	mkdir -p $DISCIMAGE/usr/local/bin/gles1tests
	if test -d $EURASIAROOT/eurasiacon/opengles1/tests; then

		for test in $(find $EURASIAROOT/eurasiacon/opengles1/tests -regex '.*makefile.linux' -exec cat {} \;|egrep -r 'MODULE\W*='|dos2unix|cut -d '=' -f2)
		do
			if test -f $test; then
				install_file $test /usr/local/bin/gles1tests/$test "binary" 0755 0:0
			fi
		done
	fi

popdivert()]])


###############################################################################
##
## STANDARD_KERNEL_MODULES
## Install all regular kernel modules - the PVR services module (includes the
## SGX driver) and the display driver, if one is defined.  We also do a run of
## depmod(8) if the mode requires.
##
## No parameters.
##
###############################################################################
define([[STANDARD_KERNEL_MODULES]], [[pushdivert(52)dnl
	# Check the kernel module directory is there
	#
	if [ ! -d "${DISCIMAGE}MOD_ROOTDIR" ]; then
		echo ""
		echo "Can't find MOD_ROOTDIR on file system installation root"
		echo -n "There is no kernel module area setup yet. "
		if [ "$from" = target ]; then
			echo "On your build machine you should invoke:"
			echo
			echo " $ cd \$KERNELDIR"
			echo " $ make INSTALL_MOD_PATH=\$DISCIMAGE modules_install"
		else
			echo "You should invoke:"
			echo
			echo " $ cd $KERNELDIR"
			echo " $ make INSTALL_MOD_PATH=$DISCIMAGE modules_install"
		fi
		echo
		exit 1;
	fi

	# Install the standard kernel modules
	# Touch some files that might not exist so that busybox/modprobe don't complain
	#
	ifelse(TARGET_HAS_DEPMOD,FALSE,[[cp MODULES_DEP MODULES_DEP[[.old]]
]])dnl

ifelse(SUPPORT_DRI_DRM_NOT_PCI,1,[[dnl
KERNEL_MODULE(DRM_MODNAME.KM_SUFFIX)dnl
KERNEL_MODULE(PVRSRV_MODNAME.KM_SUFFIX, [[DRM_MODNAME.KM_SUFFIX]])dnl
]],[[dnl
KERNEL_MODULE(PVRSRV_MODNAME.KM_SUFFIX)dnl
ifelse(PDUMP,1,[[KERNEL_MODULE([[dbgdrv.KM_SUFFIX]])]])dnl
]])dnl

ifdef([[_DISPLAY_CONTROLLER_]],
	if [ -z "$NO_DISPLAYMOD" ]; then
	KERNEL_MODULE(_DISPLAY_CONTROLLER_[[.]]KM_SUFFIX, [[PVRSRV_MODNAME.KM_SUFFIX]])
fi)

ifdef([[_BUFFER_CLASS_DEVICE_]],
	if [ -z "$NO_BCDEVICE" ]; then
	KERNEL_MODULE(_BUFFER_CLASS_DEVICE_[[.]]KM_SUFFIX, [[PVRSRV_MODNAME.KM_SUFFIX]])
fi)

ifelse(SUPPORT_ANDROID_PLATFORM,1,,[[
	$DOIT touch LIBMODPROBE_CONF
	$DOIT touch MODULES_CONF
	$DOIT rm -f MODULES_TMP
]])

popdivert()]])

###############################################################################
##
## STANDARD_HEADERS
## Install headers
##
## No parameters.
##
###############################################################################
define([[STANDARD_HEADERS]], [[pushdivert(52)dnl
ifelse(SUPPORT_EWS,1,[[dnl
HEADER(ews.h, ews)dnl
HEADER(ews_types.h, ews)dnl
]])dnl
popdivert()]])

###############################################################################
##
## XORG_ALL -
##
## No parameters.
##
###############################################################################
define([[XORG_ALL]], [[pushdivert(54)dnl
install_X()
{
popdivert()dnl
pushdivert(55)dnl
ifelse(LWS_INSTALL_TREE,1,[[dnl
	[ -d usr ] &&
		install_tree usr/local/XSGX /usr/local/XSGX "Linux window system executables and libraries"
]])dnl
ifelse(SUPPORT_XORG,1,[[dnl
	[ -f pvr_drv.so ] &&
		install_file pvr_drv.so PVR_DDX_DESTDIR/pvr_drv.so "X.Org PVR DDX video module" 0755 0:0
 ifelse(SUPPORT_XORG_CONF,1,[[dnl
	[ -f xorg.conf ] &&
		install_file xorg.conf XORG_DIR/etc/xorg.conf "X.Org configuration file" 0644 0:0
 ]])dnl
]])dnl
popdivert()dnl
pushdivert(56)dnl
}
popdivert()dnl
pushmain()dnl
[ -z "$NO_X" ] && install_X
popdivert()]])

###############################################################################
##
## LWS_ALL -
##
## No parameters.
##
###############################################################################
define([[LWS_ALL]], [[XORG_ALL]])dnl

###############################################################################
##
## DRM_ALL -
##
## No parameters.
##
###############################################################################
define([[DRM_ALL]], [[pushdivert(54)dnl
install_drm()
{
popdivert()dnl
pushdivert(55)dnl
ifelse(SUPPORT_DRI_DRM,1,[[dnl
	[ -d usr ] &&
		install_tree usr/local/XSGX /usr/local/XSGX "DRM libraries"
]])dnl
popdivert()dnl
pushdivert(56)dnl
}
popdivert()dnl
pushmain()dnl
[ -z "$NO_DRM" ] && install_drm
popdivert()]])
