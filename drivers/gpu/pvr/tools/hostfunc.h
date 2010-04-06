/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful but, except
 * as otherwise stated in writing, without any warranty; without even the
 * implied warranty of merchantability or fitness for a particular purpose.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK
 *
 ******************************************************************************/

#ifndef _HOSTFUNC_
#define _HOSTFUNC_

#define HOST_PAGESIZE			(4096)
#define DBG_MEMORY_INITIALIZER	(0xe2)

u32 HostReadRegistryDWORDFromString(char *pcKey, char *pcValueName,
					   u32 *pui32Data);

void *HostPageablePageAlloc(u32 ui32Pages);
void HostPageablePageFree(void *pvBase);
void *HostNonPageablePageAlloc(u32 ui32Pages);
void HostNonPageablePageFree(void *pvBase);

void *HostMapKrnBufIntoUser(void *pvKrnAddr, u32 ui32Size, void **ppvMdl);
void HostUnMapKrnBufFromUser(void *pvUserAddr, void *pvMdl, void *pvProcess);

void HostCreateRegDeclStreams(void);

void *HostCreateMutex(void);
void HostAquireMutex(void *pvMutex);
void HostReleaseMutex(void *pvMutex);
void HostDestroyMutex(void *pvMutex);

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
s32 HostCreateEventObjects(void);
void HostWaitForEvent(enum DBG_EVENT eEvent);
void HostSignalEvent(enum DBG_EVENT eEvent);
void HostDestroyEventObjects(void);
#endif

#endif
