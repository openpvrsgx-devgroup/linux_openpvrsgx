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

#ifndef __PVR_DEBUG_H__
#define __PVR_DEBUG_H__


#include "img_types.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define PVR_MAX_DEBUG_MESSAGE_LEN	(512)

#define DBGPRIV_FATAL		0x01UL
#define DBGPRIV_ERROR		0x02UL
#define DBGPRIV_WARNING		0x04UL
#define DBGPRIV_MESSAGE		0x08UL
#define DBGPRIV_VERBOSE		0x10UL
#define DBGPRIV_CALLTRACE	0x20UL
#define DBGPRIV_ALLOC		0x40UL
#define DBGPRIV_ALLLEVELS	(DBGPRIV_FATAL | DBGPRIV_ERROR | DBGPRIV_WARNING | DBGPRIV_MESSAGE | DBGPRIV_VERBOSE)



#define PVR_DBG_FATAL		DBGPRIV_FATAL,__FILE__, __LINE__
#define PVR_DBG_ERROR		DBGPRIV_ERROR,__FILE__, __LINE__
#define PVR_DBG_WARNING		DBGPRIV_WARNING,__FILE__, __LINE__
#define PVR_DBG_MESSAGE		DBGPRIV_MESSAGE,__FILE__, __LINE__
#define PVR_DBG_VERBOSE		DBGPRIV_VERBOSE,__FILE__, __LINE__
#define PVR_DBG_CALLTRACE	DBGPRIV_CALLTRACE,__FILE__, __LINE__
#define PVR_DBG_ALLOC		DBGPRIV_ALLOC,__FILE__, __LINE__

#if !defined(PVRSRV_NEED_PVR_ASSERT) && defined(DEBUG)
#define PVRSRV_NEED_PVR_ASSERT
#endif

#if defined(PVRSRV_NEED_PVR_ASSERT) && !defined(PVRSRV_NEED_PVR_DPF)
#define PVRSRV_NEED_PVR_DPF
#endif

#if !defined(PVRSRV_NEED_PVR_TRACE) && (defined(DEBUG) || defined(TIMING))
#define PVRSRV_NEED_PVR_TRACE
#endif


#if defined(PVRSRV_NEED_PVR_ASSERT)

	#define PVR_ASSERT(EXPR) if (!(EXPR)) PVRSRVDebugAssertFail(__FILE__, __LINE__);

IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVDebugAssertFail(const IMG_CHAR *pszFile,
													   IMG_UINT32 ui32Line);

			#if defined(PVR_DBG_BREAK_ASSERT_FAIL)
				#define PVR_DBG_BREAK	PVRSRVDebugAssertFail("PVR_DBG_BREAK", 0)
			#else
				#define PVR_DBG_BREAK
			#endif

#else

	#define PVR_ASSERT(EXPR)
	#define PVR_DBG_BREAK

#endif


#if defined(PVRSRV_NEED_PVR_DPF)

	#define PVR_DPF(X)		PVRSRVDebugPrintf X

IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVDebugPrintf(IMG_UINT32 ui32DebugLevel,
												   const IMG_CHAR *pszFileName,
												   IMG_UINT32 ui32Line,
												   const IMG_CHAR *pszFormat,
												   ...);

#else

	#define PVR_DPF(X)

#endif


#if defined(PVRSRV_NEED_PVR_TRACE)

	#define PVR_TRACE(X)	PVRSRVTrace X

IMG_IMPORT IMG_VOID IMG_CALLCONV PVRSRVTrace(const IMG_CHAR* pszFormat, ... );

#else

	#define PVR_TRACE(X)

#endif


#if defined (__cplusplus)
}
#endif

#endif

