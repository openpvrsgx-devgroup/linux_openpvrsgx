/*************************************************************************/ /*!
@File
@Title          Version numbers and strings.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Version numbers and strings for PVR Consumer services
                components.
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#ifndef _PVRVERSION_H_
#define _PVRVERSION_H_

#define INVOKE(F, ...) F(__VA_ARGS__)
#define CONCAT(X, Y) X##Y
#define PVR_STR(X) #X
#define PVR_STR2(X) PVR_STR(X)

/* 0 means the highest supported version. */
#define PVR_ABI_IMPL2(major, minor, build) ((minor << 24 | build) - 1)
#define PVR_ABI_IMPL(major, minor, build, ...) (INVOKE(PVR_ABI_IMPL2, CONCAT(major, U), CONCAT(minor, U), CONCAT(build, U)))
#define PVR_ABI_VERSION(...) PVR_ABI_IMPL(__VA_ARGS__, 0, 0)

#define PVRVERSION_MAJ               1
#if PVR_ABI_VERSION(PVR_ABI_COMPAT) == PVR_ABI_VERSION(1,7,862890)
#define PVRVERSION_MIN               7
#define PVRVERSION_BRANCH            17
#else
#define PVRVERSION_MIN               17
#define PVRVERSION_BRANCH            0
#endif

#define PVRVERSION_FAMILY           "sgxddk"
#define PVRVERSION_BRANCHNAME       "1.17"
#define PVRVERSION_BUILD            INVOKE(CONCAT, PVRVERSION_BUILD_HI, PVRVERSION_BUILD_LO)
#define PVRVERSION_BSCONTROL        "SGX_DDK"

#define PVRVERSION_STRING           "SGX_DDK sgxddk 1.17@" PVR_STR2(PVRVERSION_BUILD)
#define PVRVERSION_STRING_SHORT     "1.17@" PVR_STR2(PVRVERSION_BUILD) ""

#define COPYRIGHT_TXT               "Copyright (c) Imagination Technologies Ltd. All Rights Reserved."

#if PVR_ABI_VERSION(PVR_ABI_COMPAT) == PVR_ABI_VERSION(1,7,862890)
#define PVRVERSION_BUILD_HI          86
#define PVRVERSION_BUILD_LO          2890
#else
#define PVRVERSION_BUILD_HI          494
#define PVRVERSION_BUILD_LO          8957
#endif
#define PVRVERSION_STRING_NUMERIC    PVR_STR2(PVRVERSION_MAJ) "." PVR_STR2(PVRVERSION_MIN) "." PVR_STR2(PVRVERSION_BUILD_HI) "." PVR_STR2(PVRVERSION_BUILD_LO)

#endif /* _PVRVERSION_H_ */
