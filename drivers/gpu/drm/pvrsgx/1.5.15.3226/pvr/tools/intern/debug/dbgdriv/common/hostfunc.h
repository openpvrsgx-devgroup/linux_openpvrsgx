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

#ifndef _HOSTFUNC_
#define _HOSTFUNC_

#define HOST_PAGESIZE			(4096)
#define DBG_MEMORY_INITIALIZER	(0xe2)

IMG_UINT32 HostReadRegistryDWORDFromString(IMG_CHAR *pcKey, IMG_CHAR *pcValueName, IMG_UINT32 *pui32Data);

IMG_VOID * HostPageablePageAlloc(IMG_UINT32 ui32Pages);
IMG_VOID HostPageablePageFree(IMG_VOID * pvBase);
IMG_VOID * HostNonPageablePageAlloc(IMG_UINT32 ui32Pages);
IMG_VOID HostNonPageablePageFree(IMG_VOID * pvBase);

IMG_VOID * HostMapKrnBufIntoUser(IMG_VOID * pvKrnAddr, IMG_UINT32 ui32Size, IMG_VOID * *ppvMdl);
IMG_VOID HostUnMapKrnBufFromUser(IMG_VOID * pvUserAddr, IMG_VOID * pvMdl, IMG_VOID * pvProcess);

IMG_VOID HostCreateRegDeclStreams(IMG_VOID);

IMG_VOID * HostCreateMutex(IMG_VOID);
IMG_VOID HostAquireMutex(IMG_VOID * pvMutex);
IMG_VOID HostReleaseMutex(IMG_VOID * pvMutex);
IMG_VOID HostDestroyMutex(IMG_VOID * pvMutex);

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
IMG_INT32 HostCreateEventObjects(IMG_VOID);
IMG_VOID HostWaitForEvent(DBG_EVENT eEvent);
IMG_VOID HostSignalEvent(DBG_EVENT eEvent);
IMG_VOID HostDestroyEventObjects(IMG_VOID);
#endif

#endif

