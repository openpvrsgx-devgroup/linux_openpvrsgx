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

#ifndef __LINKAGE_H__
#define __LINKAGE_H__

#if !defined(SUPPORT_DRI_DRM)
IMG_INT32 PVRSRV_BridgeDispatchKM(struct file *file, IMG_UINT cmd, IMG_UINT32 arg);
#endif

IMG_VOID PVRDPFInit(IMG_VOID);
PVRSRV_ERROR PVROSFuncInit(IMG_VOID);
IMG_VOID PVROSFuncDeInit(IMG_VOID);

#ifdef DEBUG
IMG_INT PVRDebugProcSetLevel(struct file *file, const IMG_CHAR *buffer, IMG_UINT32 count, IMG_VOID *data);
IMG_VOID PVRDebugSetLevel(IMG_UINT32 uDebugLevel);

#ifdef PVR_PROC_USE_SEQ_FILE
void ProcSeqShowDebugLevel(struct seq_file *sfile,void* el);
#else
IMG_INT PVRDebugProcGetLevel(IMG_CHAR *page, IMG_CHAR **start, off_t off, IMG_INT count, IMG_INT *eof, IMG_VOID *data);
#endif

#ifdef PVR_MANUAL_POWER_CONTROL
IMG_INT PVRProcSetPowerLevel(struct file *file, const IMG_CHAR *buffer, IMG_UINT32 count, IMG_VOID *data);

#ifdef PVR_PROC_USE_SEQ_FILE
void ProcSeqShowPowerLevel(struct seq_file *sfile,void* el);
#else
IMG_INT PVRProcGetPowerLevel(IMG_CHAR *page, IMG_CHAR **start, off_t off, IMG_INT count, IMG_INT *eof, IMG_VOID *data);
#endif


#endif
#endif

#endif
