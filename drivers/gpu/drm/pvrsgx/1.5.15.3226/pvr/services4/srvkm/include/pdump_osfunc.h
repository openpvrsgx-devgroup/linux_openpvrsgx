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

#ifndef __PDUMP_OSFUNC_H__
#define __PDUMP_OSFUNC_H__

#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif


#define MAX_PDUMP_STRING_LENGTH (256)
#define PDUMP_GET_SCRIPT_STRING()				\
	IMG_HANDLE hScript;							\
	IMG_UINT32	ui32MaxLen;						\
	PVRSRV_ERROR eError;						\
	eError = PDumpOSGetScriptString(&hScript, &ui32MaxLen);\
	if(eError != PVRSRV_OK) return eError;

#define PDUMP_GET_MSG_STRING()					\
	IMG_HANDLE hMsg;							\
	IMG_UINT32	ui32MaxLen;						\
	PVRSRV_ERROR eError;						\
	eError = PDumpOSGetMessageString(&hMsg, &ui32MaxLen);\
	if(eError != PVRSRV_OK) return eError;

#define PDUMP_GET_FILE_STRING()				\
	IMG_CHAR *pszFileName;					\
	IMG_UINT32	ui32MaxLen;					\
	PVRSRV_ERROR eError;					\
	eError = PDumpOSGetFilenameString(&pszFileName, &ui32MaxLen);\
	if(eError != PVRSRV_OK) return eError;

#define PDUMP_GET_SCRIPT_AND_FILE_STRING()		\
	IMG_HANDLE hScript;							\
	IMG_CHAR *pszFileName;						\
	IMG_UINT32	ui32MaxLenScript;				\
	IMG_UINT32	ui32MaxLenFileName;				\
	PVRSRV_ERROR eError;						\
	eError = PDumpOSGetScriptString(&hScript, &ui32MaxLenScript);\
	if(eError != PVRSRV_OK) return eError;		\
	eError = PDumpOSGetFilenameString(&pszFileName, &ui32MaxLenFileName);\
	if(eError != PVRSRV_OK) return eError;



	PVRSRV_ERROR PDumpOSGetScriptString(IMG_HANDLE *phScript, IMG_UINT32 *pui32MaxLen);


	PVRSRV_ERROR PDumpOSGetMessageString(IMG_HANDLE *phMsg, IMG_UINT32 *pui32MaxLen);


	PVRSRV_ERROR PDumpOSGetFilenameString(IMG_CHAR **ppszFile, IMG_UINT32 *pui32MaxLen);




#define PDUMP_va_list	va_list
#define PDUMP_va_start	va_start
#define PDUMP_va_end	va_end



IMG_HANDLE PDumpOSGetStream(IMG_UINT32 ePDumpStream);

IMG_UINT32 PDumpOSGetStreamOffset(IMG_UINT32 ePDumpStream);

IMG_UINT32 PDumpOSGetParamFileNum(IMG_VOID);

IMG_VOID PDumpOSCheckForSplitting(IMG_HANDLE hStream, IMG_UINT32 ui32Size, IMG_UINT32 ui32Flags);

IMG_BOOL PDumpOSIsSuspended(IMG_VOID);

IMG_BOOL PDumpOSJTInitialised(IMG_VOID);

IMG_BOOL PDumpOSWriteString(IMG_HANDLE hDbgStream,
		IMG_UINT8 *psui8Data,
		IMG_UINT32 ui32Size,
		IMG_UINT32 ui32Flags);

IMG_BOOL PDumpOSWriteString2(IMG_HANDLE	hScript, IMG_UINT32 ui32Flags);

PVRSRV_ERROR PDumpOSBufprintf(IMG_HANDLE hBuf, IMG_UINT32 ui32ScriptSizeMax, IMG_CHAR* pszFormat, ...);

IMG_VOID PDumpOSDebugPrintf(IMG_CHAR* pszFormat, ...);

PVRSRV_ERROR PDumpOSSprintf(IMG_CHAR *pszComment, IMG_UINT32 ui32ScriptSizeMax, IMG_CHAR *pszFormat, ...);

PVRSRV_ERROR PDumpOSVSprintf(IMG_CHAR *pszMsg, IMG_UINT32 ui32ScriptSizeMax, IMG_CHAR* pszFormat, PDUMP_va_list vaArgs);

IMG_UINT32 PDumpOSBuflen(IMG_HANDLE hBuffer, IMG_UINT32 ui32BufferSizeMax);

IMG_VOID PDumpOSVerifyLineEnding(IMG_HANDLE hBuffer, IMG_UINT32 ui32BufferSizeMax);

IMG_VOID PDumpOSCPUVAddrToDevPAddr(PVRSRV_DEVICE_TYPE eDeviceType,
        IMG_HANDLE hOSMemHandle,
		IMG_UINT32 ui32Offset,
		IMG_UINT8 *pui8LinAddr,
		IMG_UINT32 ui32PageSize,
		IMG_DEV_PHYADDR *psDevPAddr);

IMG_VOID PDumpOSCPUVAddrToPhysPages(IMG_HANDLE hOSMemHandle,
		IMG_UINT32 ui32Offset,
		IMG_PUINT8 pui8LinAddr,
		IMG_UINT32 *pui32PageOffset);

#if defined (__cplusplus)
}
#endif

#endif

