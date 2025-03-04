/*************************************************************************/ /*!
@Title          OMAP Linux display driver shared DRM structures
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    OMAP Linux display driver DRM structures shared between
                kernel and user space.
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
#ifndef __3RDPARTY_DC_DRM_SHARED_H__
#define __3RDPARTY_DC_DRM_SHARED_H__
#if defined(SUPPORT_DRI_DRM)

#define	PVR_DRM_DISP_CMD_ENTER_VT	1
#define	PVR_DRM_DISP_CMD_LEAVE_VT	2

#define	PVR_DRM_DISP_CMD_ON		3
#define	PVR_DRM_DISP_CMD_STANDBY	4
#define	PVR_DRM_DISP_CMD_SUSPEND	5
#define	PVR_DRM_DISP_CMD_OFF		6

#define	PVR_DRM_DISP_ARG_CMD		0
#define	PVR_DRM_DISP_ARG_DEV		1
#define	PVR_DRM_DISP_NUM_ARGS		2

#endif	/* defined(SUPPORT_DRI_DRM) */
#endif /* __3RDPARTY_DC_DRM_SHARED_H__ */

/******************************************************************************
 End of file (3rdparty_dc_drm_shared.h)
******************************************************************************/
