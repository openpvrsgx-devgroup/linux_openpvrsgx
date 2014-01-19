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

#ifndef _IOCTL_
#define _IOCTL_


IMG_UINT32 DBGDIOCDrivCreateStream(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivDestroyStream(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivGetStream(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivWriteString(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivReadString(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivWrite(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivWrite2(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivRead(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivSetCaptureMode(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivSetOutMode(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivSetDebugLevel(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivSetFrame(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivGetFrame(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivOverrideMode(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivDefaultMode(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivGetServiceTable(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivWriteStringCM(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivWriteCM(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivSetMarker(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivGetMarker(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivIsCaptureFrame(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivWriteLF(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivReadLF(IMG_VOID *, IMG_VOID *);
IMG_UINT32 DBGDIOCDrivWaitForEvent(IMG_VOID*, IMG_VOID *);

IMG_UINT32 (*g_DBGDrivProc[])(IMG_VOID *, IMG_VOID *) =
{
	DBGDIOCDrivCreateStream,
	DBGDIOCDrivDestroyStream,
	DBGDIOCDrivGetStream,
	DBGDIOCDrivWriteString,
	DBGDIOCDrivReadString,
	DBGDIOCDrivWrite,
	DBGDIOCDrivRead,
	DBGDIOCDrivSetCaptureMode,
	DBGDIOCDrivSetOutMode,
	DBGDIOCDrivSetDebugLevel,
	DBGDIOCDrivSetFrame,
	DBGDIOCDrivGetFrame,
	DBGDIOCDrivOverrideMode,
	DBGDIOCDrivDefaultMode,
	DBGDIOCDrivGetServiceTable,
	DBGDIOCDrivWrite2,
	DBGDIOCDrivWriteStringCM,
	DBGDIOCDrivWriteCM,
	DBGDIOCDrivSetMarker,
	DBGDIOCDrivGetMarker,
	DBGDIOCDrivIsCaptureFrame,
	DBGDIOCDrivWriteLF,
	DBGDIOCDrivReadLF,
	DBGDIOCDrivWaitForEvent
};

#define MAX_DBGVXD_W32_API (sizeof(g_DBGDrivProc)/sizeof(IMG_UINT32))

#endif

