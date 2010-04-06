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

#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "pdump_km.h"
#include "sgxinfokm.h"
#include "syslocal.h"
#include "sysconfig.h"
#include "syscommon.h"
#include "img_types.h"
#include "ocpdefs.h"
#include "pvr_bridge_km.h"

struct SYS_DATA *gpsSysData;
static struct SYS_DATA gsSysData;

static struct SYS_SPECIFIC_DATA gsSysSpecificData;
struct SYS_SPECIFIC_DATA *gpsSysSpecificData;

static u32 gui32SGXDeviceID;
static struct SGX_DEVICE_MAP gsSGXDeviceMap;
static struct PVRSRV_DEVICE_NODE *gpsSGXDevNode;

#define DEVICE_SGX_INTERRUPT	(1 << 0)

#if defined(NO_HARDWARE)
static void *gsSGXRegsCPUVAddr;
#endif

static void __iomem *ocp_base;

static enum PVRSRV_ERROR SysLocateDevices(struct SYS_DATA *psSysData)
{
#if defined(NO_HARDWARE)
	enum PVRSRV_ERROR eError;
	struct IMG_CPU_PHYADDR sCpuPAddr;
#endif

	PVR_UNREFERENCED_PARAMETER(psSysData);

	gsSGXDeviceMap.ui32Flags = 0x0;

#if defined(NO_HARDWARE)

	eError = OSBaseAllocContigMemory(SYS_OMAP3430_SGX_REGS_SIZE,
					 &gsSGXRegsCPUVAddr, &sCpuPAddr);
	if (eError != PVRSRV_OK)
		return eError;
	gsSGXDeviceMap.sRegsCpuPBase = sCpuPAddr;
	gsSGXDeviceMap.sRegsSysPBase =
	    SysCpuPAddrToSysPAddr(gsSGXDeviceMap.sRegsCpuPBase);
	gsSGXDeviceMap.ui32RegsSize = SYS_OMAP3430_SGX_REGS_SIZE;

	gsSGXDeviceMap.pvRegsCpuVBase = gsSGXRegsCPUVAddr;

	OSMemSet(gsSGXRegsCPUVAddr, 0, SYS_OMAP3430_SGX_REGS_SIZE);

	gsSGXDeviceMap.ui32IRQ = 0;

#else

	gsSGXDeviceMap.sRegsSysPBase.uiAddr =
	    SYS_OMAP3430_SGX_REGS_SYS_PHYS_BASE;
	gsSGXDeviceMap.sRegsCpuPBase =
	    SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	gsSGXDeviceMap.ui32RegsSize = SYS_OMAP3430_SGX_REGS_SIZE;

	gsSGXDeviceMap.ui32IRQ = SYS_OMAP3430_SGX_IRQ;

#endif

	return PVRSRV_OK;
}

#ifndef NO_HARDWARE
u32 sgx_get_rev(void)
{
	/*
	 * Ugly solution, used until we have proper per device instances
	 * passed to functions and get rid of most if not all globals.
	 */
	struct SYS_SPECIFIC_DATA *sysd = gpsSysSpecificData;
	void __iomem *regs;
	static u32 rev = -1UL;
	int err;

	if (rev != -1UL)
		return rev;

	regs = OSMapPhysToLin(gsSGXDeviceMap.sRegsCpuPBase,
			       SYS_OMAP3430_SGX_REGS_SIZE,
			       PVRSRV_HAP_UNCACHED | PVRSRV_HAP_KERNEL_ONLY,
			       NULL);
	if (!regs)
		return 0;

	err = clk_enable(sysd->psSGX_FCK);
	BUG_ON(err);
	err = clk_enable(sysd->psSGX_ICK);
	BUG_ON(err);

	rev = OSReadHWReg(regs, EUR_CR_CORE_REVISION);

	clk_disable(sysd->psSGX_ICK);
	clk_disable(sysd->psSGX_FCK);

	OSUnMapPhysToLin(regs, SYS_OMAP3430_SGX_REGS_SIZE,
			 PVRSRV_HAP_UNCACHED | PVRSRV_HAP_KERNEL_ONLY, NULL);

	return rev;
}

unsigned long sgx_get_max_freq(void)
{
	if (sgx_is_530()) {
		switch (sgx_get_rev()) {
		case EUR_CR_CORE_MAKE_REV(1, 2, 1):
			return SYS_SGX_MAX_FREQ_530R121;
		case EUR_CR_CORE_MAKE_REV(1, 2, 5):
			return SYS_SGX_MAX_FREQ_530R125;
		}
	}
	BUG();
}

#else

u32 sgx_get_rev(void)
{
	return 0;
}

unsigned long sgx_get_max_freq()
{
	return SYS_SGX_MAX_FREQ_NO_HW;
}

#endif

bool sgx_is_530(void)
{
#ifdef SGX530
	return true;
#endif
	return false;
}

char *SysCreateVersionString(struct IMG_CPU_PHYADDR sRegRegion)
{
	static char aszVersionString[100];
	struct SYS_DATA *psSysData;
	u32 ui32SGXRevision;
	s32 i32Count;

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		return NULL;

	ui32SGXRevision = sgx_get_rev();

	i32Count = OSSNPrintf(aszVersionString, 100,
			      "SGX revision = %u.%u.%u",
			      (unsigned)((ui32SGXRevision &
				     EUR_CR_CORE_REVISION_MAJOR_MASK)
				    >> EUR_CR_CORE_REVISION_MAJOR_SHIFT),
			      (unsigned)((ui32SGXRevision &
				     EUR_CR_CORE_REVISION_MINOR_MASK)
				    >> EUR_CR_CORE_REVISION_MINOR_SHIFT),
			      (unsigned)((ui32SGXRevision &
				     EUR_CR_CORE_REVISION_MAINTENANCE_MASK)
				    >> EUR_CR_CORE_REVISION_MAINTENANCE_SHIFT)
	    );

	if (i32Count == -1)
		return NULL;

	return aszVersionString;
}

static int sgx_ocp_init(void)
{
	struct IMG_SYS_PHYADDR sys_pbase;
	struct IMG_CPU_PHYADDR cpu_pbase;

	sys_pbase.uiAddr = SYS_OMAP3430_OCP_REGS_SYS_PHYS_BASE;
	cpu_pbase = SysSysPAddrToCpuPAddr(sys_pbase);

	ocp_base = OSMapPhysToLin(cpu_pbase, SYS_OMAP3430_OCP_REGS_SIZE,
				  PVRSRV_HAP_UNCACHED | PVRSRV_HAP_KERNEL_ONLY,
				  NULL);

	if (!ocp_base) {
		PVR_DPF(PVR_DBG_ERROR, "%s: Failed to map OCP registers",
			__func__);
		return -ENOMEM;
	}

	return 0;
}

static void sgx_ocp_cleanup(void)
{
	OSUnMapPhysToLin(ocp_base, SYS_OMAP3430_OCP_REGS_SIZE,
			 PVRSRV_HAP_UNCACHED | PVRSRV_HAP_KERNEL_ONLY, NULL);
}

void sgx_ocp_write_reg(u32 reg, u32 val)
{
	BUG_ON(!ocp_base);

	/* OCP offsets are based at EUR_CR_OCP_REVISION */
	reg -= EUR_CR_OCP_REVISION;
	OSWriteHWReg(ocp_base, reg, val);
}

enum PVRSRV_ERROR SysInitialise(void)
{
	u32 i;
	enum PVRSRV_ERROR eError;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct IMG_CPU_PHYADDR TimerRegPhysBase;

	gpsSysData = &gsSysData;

	gpsSysSpecificData = &gsSysSpecificData;

	gpsSysData->pvSysSpecificData = gpsSysSpecificData;

	eError = OSInitEnvData(&gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed to setup env structure");
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
			      SYS_SPECIFIC_DATA_ENABLE_ENVDATA);

	gpsSysData->ui32NumDevices = SYS_DEVICE_COUNT;

	for (i = 0; i < SYS_DEVICE_COUNT; i++) {
		gpsSysData->sDeviceID[i].uiID = i;
		gpsSysData->sDeviceID[i].bInUse = IMG_FALSE;
	}

	gpsSysData->psDeviceNodeList = NULL;
	gpsSysData->psQueueList = NULL;

	eError = SysInitialiseCommon(gpsSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed in SysInitialiseCommon");
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}

	TimerRegPhysBase.uiAddr =
	    SYS_OMAP3430_GP11TIMER_PHYS_BASE + SYS_OMAP3430_GPTIMER_REGS;
	gpsSysData->pvSOCTimerRegisterKM = NULL;
	gpsSysData->hSOCTimerRegisterOSMemHandle = NULL;
	OSReservePhys(TimerRegPhysBase, 4,
		      PVRSRV_HAP_MULTI_PROCESS | PVRSRV_HAP_UNCACHED,
		      (void **)&gpsSysData->pvSOCTimerRegisterKM,
		      &gpsSysData->hSOCTimerRegisterOSMemHandle);

	gpsSysSpecificData->ui32SrcClockDiv = 3;

	eError = SysLocateDevices(gpsSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed to locate devices");
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
			      SYS_SPECIFIC_DATA_ENABLE_LOCATEDEV);

	if (sgx_ocp_init() < 0) {
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;

		return PVRSRV_ERROR_GENERIC;
	}

	eError = PVRSRVRegisterDevice(gpsSysData, SGXRegisterDevice,
				      DEVICE_SGX_INTERRUPT, &gui32SGXDeviceID);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed to register device!");
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
			      SYS_SPECIFIC_DATA_ENABLE_REGDEV);

	psDeviceNode = gpsSysData->psDeviceNodeList;
	while (psDeviceNode) {
		switch (psDeviceNode->sDevId.eDeviceType) {
		case PVRSRV_DEVICE_TYPE_SGX:
			{
				struct DEVICE_MEMORY_INFO *psDevMemoryInfo;
				struct DEVICE_MEMORY_HEAP_INFO
							*psDeviceMemoryHeap;

				psDeviceNode->psLocalDevMemArena = NULL;

				psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
				psDeviceMemoryHeap =
				    psDevMemoryInfo->psDeviceMemoryHeap;

				for (i = 0; i < psDevMemoryInfo->ui32HeapCount;
				     i++)
					psDeviceMemoryHeap[i].ui32Attribs |=
					   PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG;

				gpsSGXDevNode = psDeviceNode;
				gsSysSpecificData.psSGXDevNode = psDeviceNode;

				break;
			}
		default:
			PVR_DPF(PVR_DBG_ERROR, "SysInitialise: "
					"Failed to find SGX device node!");
			return PVRSRV_ERROR_INIT_FAILURE;
		}

		psDeviceNode = psDeviceNode->psNext;
	}

	PDUMPINIT();
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
			      SYS_SPECIFIC_DATA_ENABLE_PDUMPINIT);

	eError = InitSystemClocks(gpsSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed to init system clocks (%d)",
			 eError);
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}

	eError = EnableSystemClocks(gpsSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed to Enable system clocks (%d)",
			 eError);
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
			      SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS);

	eError = OSInitPerf(gpsSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed to init DVFS (%d)", eError);
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
			      SYS_SPECIFIC_DATA_ENABLE_PERF);

	eError = EnableSGXClocks(gpsSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed to Enable SGX clocks (%d)",
			 eError);
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}

	eError = PVRSRVInitialiseDevice(gui32SGXDeviceID);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed to initialise device!");
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
			      SYS_SPECIFIC_DATA_ENABLE_INITDEV);

	gpsSysData->pszVersionString =
	    SysCreateVersionString(gsSGXDeviceMap.sRegsCpuPBase);
	if (!gpsSysData->pszVersionString)
		PVR_DPF(PVR_DBG_ERROR,
		"SysFinalise: Failed to create a system version string");
	else
		PVR_DPF(PVR_DBG_WARNING, "SysFinalise: Version string: %s",
			 gpsSysData->pszVersionString);

	DisableSGXClocks(gpsSysData);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SysFinalise(void)
{
	enum PVRSRV_ERROR eError = PVRSRV_OK;

	eError = EnableSGXClocks(gpsSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SysInitialise: Failed to Enable SGX clocks (%d)",
			 eError);
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}


	eError = OSInstallMISR(gpsSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "SysFinalise: Failed to install MISR");
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
			      SYS_SPECIFIC_DATA_ENABLE_MISR);

	eError =
	    OSInstallDeviceLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ, "SGX ISR",
				gpsSGXDevNode);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "SysFinalise: Failed to install ISR");
		SysDeinitialise(gpsSysData);
		gpsSysData = NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
			      SYS_SPECIFIC_DATA_ENABLE_LISR);

	DisableSGXClocks(gpsSysData);

	gpsSysSpecificData->bSGXInitComplete = IMG_TRUE;

	return eError;
}

enum PVRSRV_ERROR SysDeinitialise(struct SYS_DATA *psSysData)
{
	enum PVRSRV_ERROR eError;

	if (SYS_SPECIFIC_DATA_TEST
	    (gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR)) {
		eError = OSUninstallDeviceLISR(psSysData);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "SysDeinitialise: "
					"OSUninstallDeviceLISR failed");
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST
	    (gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_MISR)) {
		eError = OSUninstallMISR(psSysData);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "SysDeinitialise: "
						"OSUninstallMISR failed");
			return eError;
		}
	}

	sgx_ocp_cleanup();

	if (SYS_SPECIFIC_DATA_TEST
	    (gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_INITDEV)) {
		PVR_ASSERT(SYS_SPECIFIC_DATA_TEST
			   (gpsSysSpecificData,
			    SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS));

		eError = EnableSGXClocks(gpsSysData);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "SysDeinitialise: EnableSGXClocks failed");
			return eError;
		}

		eError = PVRSRVDeinitialiseDevice(gui32SGXDeviceID);

		DisableSGXClocks(gpsSysData);

		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "SysDeinitialise: "
					"failed to de-init the device");
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST
	    (gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS))
		DisableSystemClocks(gpsSysData);

	CleanupSystemClocks(gpsSysData);

	if (SYS_SPECIFIC_DATA_TEST(gpsSysSpecificData,
				   SYS_SPECIFIC_DATA_ENABLE_PERF)) {
		eError = OSCleanupPerf(psSysData);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "SysDeinitialise: OSCleanupDvfs failed");
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST
	    (gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_ENVDATA)) {
		eError = OSDeInitEnvData(gpsSysData->pvEnvSpecificData);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
			"SysDeinitialise: failed to de-init env structure");
			return eError;
		}
	}

	if (gpsSysData->pvSOCTimerRegisterKM)
		OSUnReservePhys(gpsSysData->pvSOCTimerRegisterKM, 4,
				PVRSRV_HAP_MULTI_PROCESS | PVRSRV_HAP_UNCACHED,
				gpsSysData->hSOCTimerRegisterOSMemHandle);

	SysDeinitialiseCommon(gpsSysData);

#if defined(NO_HARDWARE)
	if (SYS_SPECIFIC_DATA_TEST
	    (gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LOCATEDEV))

		OSBaseFreeContigMemory(SYS_OMAP3430_SGX_REGS_SIZE,
				       gsSGXRegsCPUVAddr,
				       gsSGXDeviceMap.sRegsCpuPBase);
#endif

	if (SYS_SPECIFIC_DATA_TEST
	    (gpsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_PDUMPINIT))
		PDUMPDEINIT();

	gpsSysSpecificData->ui32SysSpecificData = 0;
	gpsSysSpecificData->bSGXInitComplete = IMG_FALSE;

	gpsSysData = NULL;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SysGetDeviceMemoryMap(enum PVRSRV_DEVICE_TYPE eDeviceType,
				   void **ppvDeviceMap)
{
	switch (eDeviceType) {
	case PVRSRV_DEVICE_TYPE_SGX:
		*ppvDeviceMap = (void *) &gsSGXDeviceMap;
		break;
	default:
		PVR_DPF(PVR_DBG_ERROR, "SysGetDeviceMemoryMap: "
					"unsupported device type");
	}
	return PVRSRV_OK;
}

struct IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr(
				enum PVRSRV_DEVICE_TYPE eDeviceType,
				struct IMG_CPU_PHYADDR CpuPAddr)
{
	struct IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	DevPAddr.uiAddr = CpuPAddr.uiAddr;

	return DevPAddr;
}

struct IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr(struct IMG_SYS_PHYADDR sys_paddr)
{
	struct IMG_CPU_PHYADDR cpu_paddr;

	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}

struct IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr(struct IMG_CPU_PHYADDR cpu_paddr)
{
	struct IMG_SYS_PHYADDR sys_paddr;

	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}

struct IMG_DEV_PHYADDR SysSysPAddrToDevPAddr(
				enum PVRSRV_DEVICE_TYPE eDeviceType,
				struct IMG_SYS_PHYADDR SysPAddr)
{
	struct IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	DevPAddr.uiAddr = SysPAddr.uiAddr;

	return DevPAddr;
}

struct IMG_SYS_PHYADDR SysDevPAddrToSysPAddr(
				      enum PVRSRV_DEVICE_TYPE eDeviceType,
				      struct IMG_DEV_PHYADDR DevPAddr)
{
	struct IMG_SYS_PHYADDR SysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	SysPAddr.uiAddr = DevPAddr.uiAddr;

	return SysPAddr;
}

void SysRegisterExternalDevice(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}

void SysRemoveExternalDevice(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}

u32 SysGetInterruptSource(struct SYS_DATA *psSysData,
				 struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
#if defined(NO_HARDWARE)

	return 0xFFFFFFFF;
#else

	return psDeviceNode->ui32SOCInterruptBit;
#endif
}

void SysClearInterrupts(struct SYS_DATA *psSysData, u32 ui32ClearBits)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(ui32ClearBits);

	OSReadHWReg(((struct PVRSRV_SGXDEV_INFO *)gpsSGXDevNode->pvDevice)->
		    pvRegsBaseKM, EUR_CR_EVENT_HOST_CLEAR);
}

enum PVRSRV_ERROR SysSystemPrePowerState(enum PVR_POWER_STATE eNewPowerState)
{
	enum PVRSRV_ERROR eError = PVRSRV_OK;

	if (eNewPowerState == PVRSRV_POWER_STATE_D3) {
		PVR_TRACE("SysSystemPrePowerState: Entering state D3");

		if (SYS_SPECIFIC_DATA_TEST
		    (&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR)) {
			eError = OSUninstallDeviceLISR(gpsSysData);
			if (eError != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR,
						"SysSystemPrePowerState: "
						"OSUninstallDeviceLISR failed "
						"(%d)",
					 eError);
				return eError;
			}
			SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
					SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);
			SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData,
					SYS_SPECIFIC_DATA_ENABLE_LISR);
		}

		if (SYS_SPECIFIC_DATA_TEST
		    (&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS)) {
			DisableSystemClocks(gpsSysData);

			SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
				SYS_SPECIFIC_DATA_PM_DISABLE_SYSCLOCKS);
			SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData,
				SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS);
		}
	}

	return eError;
}

enum PVRSRV_ERROR SysSystemPostPowerState(enum PVR_POWER_STATE eNewPowerState)
{
	enum PVRSRV_ERROR eError = PVRSRV_OK;

	if (eNewPowerState == PVRSRV_POWER_STATE_D0) {
		PVR_TRACE("SysSystemPostPowerState: Entering state D0");

		if (SYS_SPECIFIC_DATA_TEST
		    (&gsSysSpecificData,
		     SYS_SPECIFIC_DATA_PM_DISABLE_SYSCLOCKS)) {
			eError = EnableSystemClocks(gpsSysData);
			if (eError != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR,
					"SysSystemPostPowerState: "
					"EnableSystemClocks failed (%d)",
					 eError);
				return eError;
			}
			SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
					SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS);
			SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData,
					SYS_SPECIFIC_DATA_PM_DISABLE_SYSCLOCKS);
		}
		if (SYS_SPECIFIC_DATA_TEST
		    (&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR)) {
			eError =
			    OSInstallDeviceLISR(gpsSysData,
						gsSGXDeviceMap.ui32IRQ,
						"SGX ISR", gpsSGXDevNode);
			if (eError != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR,
						"SysSystemPostPowerState: "
						"OSInstallDeviceLISR failed "
						"to install ISR (%d)",
					 eError);
				return eError;
			}
			SYS_SPECIFIC_DATA_SET(&gsSysSpecificData,
				SYS_SPECIFIC_DATA_ENABLE_LISR);
			SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData,
				SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);
		}
	}
	return eError;
}

enum PVRSRV_ERROR SysDevicePrePowerState(u32 ui32DeviceIndex,
				    enum PVR_POWER_STATE eNewPowerState,
				    enum PVR_POWER_STATE eCurrentPowerState)
{
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

	if (ui32DeviceIndex != gui32SGXDeviceID)
		return PVRSRV_OK;
	if (eNewPowerState == PVRSRV_POWER_STATE_D3) {
		PVR_DPF(PVR_DBG_MESSAGE,
			 "SysDevicePrePowerState: SGX Entering state D3");
		DisableSGXClocks(gpsSysData);
	}
	return PVRSRV_OK;
}

enum PVRSRV_ERROR SysDevicePostPowerState(u32 ui32DeviceIndex,
				     enum PVR_POWER_STATE eNewPowerState,
				     enum PVR_POWER_STATE eCurrentPowerState)
{
	enum PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_UNREFERENCED_PARAMETER(eNewPowerState);

	if (ui32DeviceIndex != gui32SGXDeviceID)
		return eError;
	if (eCurrentPowerState == PVRSRV_POWER_STATE_D3) {
		PVR_DPF(PVR_DBG_MESSAGE,
			 "SysDevicePostPowerState: SGX Leaving state D3");
		eError = EnableSGXClocks(gpsSysData);
	}

	return eError;
}

enum PVRSRV_ERROR SysOEMFunction(u32 ui32ID, void *pvIn, u32 ulInSize,
			    void *pvOut, u32 ulOutSize)
{
	PVR_UNREFERENCED_PARAMETER(ui32ID);
	PVR_UNREFERENCED_PARAMETER(pvIn);
	PVR_UNREFERENCED_PARAMETER(ulInSize);
	PVR_UNREFERENCED_PARAMETER(pvOut);
	PVR_UNREFERENCED_PARAMETER(ulOutSize);

	if ((ui32ID == OEM_GET_EXT_FUNCS) &&
	    (ulOutSize == sizeof(struct PVRSRV_DC_OEM_JTABLE))) {

		struct PVRSRV_DC_OEM_JTABLE *psOEMJTable =
		    (struct PVRSRV_DC_OEM_JTABLE *)pvOut;
		psOEMJTable->pfnOEMBridgeDispatch = &PVRSRV_BridgeDispatchKM;
		return PVRSRV_OK;
	}

	return PVRSRV_ERROR_INVALID_PARAMS;
}
