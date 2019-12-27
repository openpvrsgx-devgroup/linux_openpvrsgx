/*************************************************************************/ /*!
@Title          PowerVR drm driver shared definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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
#if !defined(__PVR_DRM_SHARED_H__)
#define __PVR_DRM_SHARED_H__

#if defined(SUPPORT_DRI_DRM)

/* DRM command numbers, relative to DRM_COMMAND_BASE */
#if defined(SUPPORT_DRI_DRM_EXT)
#define PVR_DRM_SRVKM_CMD	DRM_PVR_RESERVED1	/* Used for PVR Services ioctls */
#define	PVR_DRM_DISP_CMD	DRM_PVR_RESERVED2	/* Reserved for display class driver */
#define	PVR_DRM_BC_CMD		DRM_PVR_RESERVED3	/* Reserved for buffer class driver */
#define PVR_DRM_IS_MASTER_CMD	DRM_PVR_RESERVED4	/* Are we the DRM master? */
#define PVR_DRM_UNPRIV_CMD	DRM_PVR_RESERVED5	/* PVR driver unprivileged ioctls */
#define PVR_DRM_DBGDRV_CMD	DRM_PVR_RESERVED6	/* Debug driver (PDUMP) ioctls */
#else	/* defined(SUPPORT_DRI_DRM_EXT) */
#define PVR_DRM_SRVKM_CMD	0	/* Used for PVR Services ioctls */
#define	PVR_DRM_DISP_CMD	1	/* Reserved for display class driver */
#define	PVR_DRM_BC_CMD		2	/* Reserved for buffer class driver */
#define PVR_DRM_IS_MASTER_CMD	3	/* Are we the DRM master? */
#define PVR_DRM_UNPRIV_CMD	4	/* PVR driver unprivileged ioctls */
#define PVR_DRM_DBGDRV_CMD	5	/* Debug driver (PDUMP) ioctls */
#endif	/* defined(SUPPORT_DRI_DRM_EXT) */

/* Subcommands of PVR_DRM_UNPRIV_CMD */
#define	PVR_DRM_UNPRIV_INIT_SUCCESFUL	0 /* PVR Services init succesful */

#endif

#endif /* defined(__PVR_DRM_SHARED_H__) */


