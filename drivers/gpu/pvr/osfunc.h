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


#ifndef __OSFUNC_H__
#define __OSFUNC_H__

#if defined(__KERNEL__)
#include <linux/hardirq.h>
#include <linux/string.h>
#endif

#define PVRSRV_PAGEABLE_SELECT		PVRSRV_OS_PAGEABLE_HEAP

#define KERNEL_ID			0xffffffffL
#define POWER_MANAGER_ID		0xfffffffeL
#define ISR_ID				0xfffffffdL
#define TIMER_ID			0xfffffffcL

#define HOST_PAGESIZE			OSGetPageSize
#define HOST_PAGEMASK			(~(HOST_PAGESIZE()-1))

#define HOST_PAGEALIGN(addr)		(((addr) + HOST_PAGESIZE() - 1) & \
					HOST_PAGEMASK)

#define PVRSRV_OS_HEAP_MASK		0xf
#define PVRSRV_OS_PAGEABLE_HEAP		0x1
#define PVRSRV_OS_NON_PAGEABLE_HEAP	0x2

u32 OSClockus(void);
u32 OSGetPageSize(void);
enum PVRSRV_ERROR OSInstallDeviceLISR(void *pvSysData, u32 ui32Irq,
				      char *pszISRName, void *pvDeviceNode);
enum PVRSRV_ERROR OSUninstallDeviceLISR(void *pvSysData);
enum PVRSRV_ERROR OSInstallSystemLISR(void *pvSysData, u32 ui32Irq);
enum PVRSRV_ERROR OSUninstallSystemLISR(void *pvSysData);
enum PVRSRV_ERROR OSInstallMISR(void *pvSysData);
enum PVRSRV_ERROR OSUninstallMISR(void *pvSysData);
enum PVRSRV_ERROR OSInitPerf(void *pvSysData);
enum PVRSRV_ERROR OSCleanupPerf(void *pvSysData);
struct IMG_CPU_PHYADDR OSMapLinToCPUPhys(void *pvLinAddr);
void OSMemCopy(void *pvDst, void *pvSrc, u32 ui32Size);
void __iomem *OSMapPhysToLin(struct IMG_CPU_PHYADDR BasePAddr,
		     u32 ui32Bytes, u32 ui32MappingFlags, void **phOSMemHandle);
IMG_BOOL OSUnMapPhysToLin(void __iomem *pvLinAddr, u32 ui32Bytes,
			u32 ui32MappingFlags, void *hPageAlloc);

enum PVRSRV_ERROR OSReservePhys(struct IMG_CPU_PHYADDR BasePAddr, u32 ui32Bytes,
		u32 ui32Flags, void **ppvCpuVAddr, void **phOSMemHandle);
enum PVRSRV_ERROR OSUnReservePhys(void *pvCpuVAddr, u32 ui32Bytes,
		u32 ui32Flags, void *hOSMemHandle);

enum PVRSRV_ERROR OSRegisterDiscontigMem(struct IMG_SYS_PHYADDR *pBasePAddr,
		void *pvCpuVAddr, u32 ui32Bytes, u32 ui32Flags,
		void **phOSMemHandle);
enum PVRSRV_ERROR OSUnRegisterDiscontigMem(void *pvCpuVAddr, u32 ui32Bytes,
		u32 ui32Flags, void *hOSMemHandle);

static inline enum PVRSRV_ERROR OSReserveDiscontigPhys(
		struct IMG_SYS_PHYADDR *pBasePAddr, u32 ui32Bytes,
		u32 ui32Flags, void **ppvCpuVAddr, void **phOSMemHandle)
{
	*ppvCpuVAddr = NULL;
	return OSRegisterDiscontigMem(pBasePAddr, *ppvCpuVAddr, ui32Bytes,
				      ui32Flags, phOSMemHandle);
}

static inline enum PVRSRV_ERROR OSUnReserveDiscontigPhys(void *pvCpuVAddr,
			u32 ui32Bytes, u32 ui32Flags, void *hOSMemHandle)
{
	OSUnRegisterDiscontigMem(pvCpuVAddr, ui32Bytes, ui32Flags,
				hOSMemHandle);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR OSRegisterMem(struct IMG_CPU_PHYADDR BasePAddr,
		void *pvCpuVAddr, u32 ui32Bytes, u32 ui32Flags,
		void **phOSMemHandle);
enum PVRSRV_ERROR OSUnRegisterMem(void *pvCpuVAddr, u32 ui32Bytes,
		u32 ui32Flags, void *hOSMemHandle);

enum PVRSRV_ERROR OSGetSubMemHandle(void *hOSMemHandle, u32 ui32ByteOffset,
		u32 ui32Bytes, u32 ui32Flags, void **phOSMemHandleRet);
enum PVRSRV_ERROR OSReleaseSubMemHandle(void *hOSMemHandle, u32 ui32Flags);

u32 OSGetCurrentProcessIDKM(void);
u32 OSGetCurrentThreadID(void);
void OSMemSet(void *pvDest, u8 ui8Value, u32 ui32Size);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
enum PVRSRV_ERROR _OSAllocMem(u32 ui32Flags, u32 ui32Size, void **ppvLinAddr,
		void **phBlockAlloc, char *pszFilename, u32 ui32Line);
#define OSAllocMem(ui32Flags, ui32Size, ppvLinAddr, phBlockAlloc)	\
	_OSAllocMem(ui32Flags, ui32Size, ppvLinAddr, phBlockAlloc,	\
		    __FILE__, __LINE__)
void _OSFreeMem(u32 ui32Flags, u32 ui32Size, void *pvLinAddr,
		void *hBlockAlloc, char *pszFilename, u32 ui32Line);
#define OSFreeMem(ui32Flags, ui32Size, pvLinAddr, phBlockAlloc)		\
	_OSFreeMem(ui32Flags, ui32Size, pvLinAddr, phBlockAlloc,	\
		   __FILE__, __LINE__)
#else
enum PVRSRV_ERROR OSAllocMem(u32 ui32Flags, u32 ui32Size, void **ppvLinAddr,
		void **phBlockAlloc);
void OSFreeMem(u32 ui32Flags, u32 ui32Size, void *pvLinAddr, void *hBlockAlloc);
#endif
enum PVRSRV_ERROR OSAllocPages(u32 ui32Flags, u32 ui32Size, u32 ui32PageSize,
			       void **ppvLinAddr, void **phPageAlloc);
enum PVRSRV_ERROR OSFreePages(u32 ui32Flags, u32 ui32Size, void *pvLinAddr,
		void *hPageAlloc);
struct IMG_CPU_PHYADDR OSMemHandleToCpuPAddr(void *hOSMemHandle,
		u32 ui32ByteOffset);
enum PVRSRV_ERROR OSInitEnvData(void **ppvEnvSpecificData);
enum PVRSRV_ERROR OSDeInitEnvData(void *pvEnvSpecificData);
char *OSStringCopy(char *pszDest, const char *pszSrc);
s32 OSSNPrintf(char *pStr, u32 ui32Size, const char *pszFormat, ...);
#define OSStringLength(pszString) strlen(pszString)

enum PVRSRV_ERROR OSEventObjectCreate(const char *pszName,
		struct PVRSRV_EVENTOBJECT *psEventObject);
enum PVRSRV_ERROR OSEventObjectDestroy(
		struct PVRSRV_EVENTOBJECT *psEventObject);
enum PVRSRV_ERROR OSEventObjectSignal(void *hOSEventKM);
enum PVRSRV_ERROR OSEventObjectWait(void *hOSEventKM);
enum PVRSRV_ERROR OSEventObjectOpen(struct PVRSRV_EVENTOBJECT *psEventObject,
		void **phOSEvent);
enum PVRSRV_ERROR OSEventObjectClose(struct PVRSRV_EVENTOBJECT *psEventObject,
		void *hOSEventKM);

enum PVRSRV_ERROR OSBaseAllocContigMemory(u32 ui32Size, void **pLinAddr,
		struct IMG_CPU_PHYADDR *pPhysAddr);
enum PVRSRV_ERROR OSBaseFreeContigMemory(u32 ui32Size, void *LinAddr,
		struct IMG_CPU_PHYADDR PhysAddr);

void *MapUserFromKernel(void *pvLinAddrKM, u32 ui32Size, void **phMemBlock);
void *OSMapHWRegsIntoUserSpace(void *hDevCookie,
		struct IMG_SYS_PHYADDR sRegAddr, u32 ulSize, void **ppvProcess);
void OSUnmapHWRegsFromUserSpace(void *hDevCookie, void *pvUserAddr,
		void *pvProcess);

void UnmapUserFromKernel(void *pvLinAddrUM, u32 ui32Size, void *hMemBlock);

enum PVRSRV_ERROR OSMapPhysToUserSpace(void *hDevCookie,
		struct IMG_SYS_PHYADDR sCPUPhysAddr, u32 uiSizeInBytes,
		u32 ui32CacheFlags, void **ppvUserAddr, u32 *puiActualSize,
		void *hMappingHandle);

enum PVRSRV_ERROR OSUnmapPhysToUserSpace(void *hDevCookie, void *pvUserAddr,
		void *pvProcess);

enum PVRSRV_ERROR OSLockResource(struct PVRSRV_RESOURCE *psResource,
		u32 ui32ID);
enum PVRSRV_ERROR OSUnlockResource(struct PVRSRV_RESOURCE *psResource,
		u32 ui32ID);
IMG_BOOL OSIsResourceLocked(struct PVRSRV_RESOURCE *psResource, u32 ui32ID);
enum PVRSRV_ERROR OSCreateResource(struct PVRSRV_RESOURCE *psResource);
enum PVRSRV_ERROR OSDestroyResource(struct PVRSRV_RESOURCE *psResource);
void OSBreakResourceLock(struct PVRSRV_RESOURCE *psResource, u32 ui32ID);
void OSWaitus(u32 ui32Timeus);
void OSReleaseThreadQuanta(void);

u32 OSReadHWReg(void __iomem *pvLinRegBaseAddr, u32 ui32Offset);
void OSWriteHWReg(void __iomem *pvLinRegBaseAddr, u32 ui32Offset,
		  u32 ui32Value);

void *OSAddTimer(void (*pfnTimerFunc)(void *), void *pvData, u32 ui32MsTimeout);
enum PVRSRV_ERROR OSRemoveTimer(void *hTimer);
enum PVRSRV_ERROR OSEnableTimer(void *hTimer);
enum PVRSRV_ERROR OSDisableTimer(void *hTimer);

enum PVRSRV_ERROR OSGetSysMemSize(u32 *pui32Bytes);

enum HOST_PCI_INIT_FLAGS {
	HOST_PCI_INIT_FLAG_BUS_MASTER = 0x00000001,
	HOST_PCI_INIT_FLAG_MSI = 0x00000002,
	HOST_PCI_INIT_FLAG_FORCE_I32 = 0x7fffffff
};

enum PVRSRV_ERROR OSScheduleMISR(void *pvSysData);

IMG_BOOL OSProcHasPrivSrvInit(void);

enum IMG_VERIFY_TEST {
	PVR_VERIFY_WRITE = 0,
	PVR_VERIFY_READ
};

IMG_BOOL OSAccessOK(enum IMG_VERIFY_TEST eVerification,
		    const void __user *pvUserPtr, u32 ui32Bytes);

enum PVRSRV_ERROR OSCopyToUser(void *pvProcess, void __user *pvDest,
			const void *pvSrc, u32 ui32Bytes);
enum PVRSRV_ERROR OSCopyFromUser(void *pvProcess, void *pvDest,
			const void __user *pvSrc, u32 ui32Bytes);

enum PVRSRV_ERROR OSAcquirePhysPageAddr(void *pvCPUVAddr, u32 ui32Bytes,
				   struct IMG_SYS_PHYADDR *psSysPAddr,
					void **phOSWrapMem);
enum PVRSRV_ERROR OSReleasePhysPageAddr(void *hOSWrapMem);

#if defined(__KERNEL__)
#define	OS_SUPPORTS_IN_LISR
static inline IMG_BOOL OSInLISR(void unref__ * pvSysData)
{
	return in_irq();
}
#endif

#endif
