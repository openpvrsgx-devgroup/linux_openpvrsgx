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

#ifndef SRVKM_H
#define SRVKM_H


#if defined(__cplusplus)
extern "C" {
#endif


	#ifdef PVR_DISABLE_LOGGING
	#define PVR_LOG(X)
	#else
	#define PVR_LOG(X)			PVRSRVReleasePrintf X
	#endif

	IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVReleasePrintf(const IMG_CHAR *pszFormat,
										...);

	IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVProcessConnect(IMG_UINT32	ui32PID);
	IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVProcessDisconnect(IMG_UINT32	ui32PID);

	IMG_VOID IMG_CALLCONV PVRSRVSetDCState(IMG_UINT32 ui32State);

	PVRSRV_ERROR IMG_CALLCONV PVRSRVSaveRestoreLiveSegments(IMG_HANDLE hArena, IMG_PBYTE pbyBuffer, IMG_SIZE_T *puiBufSize, IMG_BOOL bSave);

#if defined (__cplusplus)
}
#endif

#define LOOP_UNTIL_TIMEOUT(TIMEOUT) \
{\
	IMG_UINT32 uiOffset, uiStart, uiCurrent, uiNotLastLoop;								\
	for(uiOffset = 0, uiStart = OSClockus(), uiCurrent = uiStart + 1, uiNotLastLoop = 1;\
		((uiCurrent - uiStart + uiOffset) < TIMEOUT) || uiNotLastLoop--;				\
		uiCurrent = OSClockus(),														\
		uiOffset = uiCurrent < uiStart ? IMG_UINT32_MAX - uiStart : uiOffset,			\
		uiStart = uiCurrent < uiStart ? 0 : uiStart)

#define END_LOOP_UNTIL_TIMEOUT() \
}


#endif
