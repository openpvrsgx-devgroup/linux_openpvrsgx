/**********************************************************************
 Copyright (c) Imagination Technologies Ltd.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ******************************************************************************/

#if !defined(__PVR_DRM_SHARED_H__)
#define __PVR_DRM_SHARED_H__

#if defined(SUPPORT_DRI_DRM)

#if defined(SUPPORT_DRI_DRM_EXT)
#define PVR_DRM_SRVKM_CMD	DRM_PVR_RESERVED1
#define	PVR_DRM_DISP_CMD	DRM_PVR_RESERVED2
#define	PVR_DRM_BC_CMD		DRM_PVR_RESERVED3
#define PVR_DRM_IS_MASTER_CMD	DRM_PVR_RESERVED4
#define PVR_DRM_UNPRIV_CMD	DRM_PVR_RESERVED5
#define PVR_DRM_DBGDRV_CMD	DRM_PVR_RESERVED6
#else
#define PVR_DRM_SRVKM_CMD	0
#define	PVR_DRM_DISP_CMD	1
#define	PVR_DRM_BC_CMD		2
#define PVR_DRM_IS_MASTER_CMD	3
#define PVR_DRM_UNPRIV_CMD	4
#define PVR_DRM_DBGDRV_CMD	5
#endif

#define	PVR_DRM_UNPRIV_INIT_SUCCESFUL	0
#define	PVR_DRM_UNPRIV_BUSID_TYPE	1
#define	PVR_DRM_UNPRIV_BUSID_FIELD	2

#define	PVR_DRM_BUS_TYPE_PCI		0

#define	PVR_DRM_PCI_DOMAIN		0
#define	PVR_DRM_PCI_BUS			1
#define	PVR_DRM_PCI_DEV			2
#define	PVR_DRM_PCI_FUNC		3

#endif

#endif


