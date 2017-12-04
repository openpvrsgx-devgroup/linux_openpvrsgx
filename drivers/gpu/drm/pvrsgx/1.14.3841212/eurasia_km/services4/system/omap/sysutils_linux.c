/*************************************************************************/ /*!
@Title          System dependent utilities
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Provides system-specific functions
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "owl_gpu_clk.h"
#include "owl_gpu_dvfs.h"

#include "sgxdefs.h"
#include "services_headers.h"
#include "sysinfo.h"
#include "sgxapi_km.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#if defined(SUPPORT_DRI_DRM_PLUGIN)
#include <drm/drmP.h>
#include <drm/drm.h>

#include <linux/omap_gpu.h>

#include "pvr_drm.h"
#endif

#define	ONE_MHZ	1000000
#define	HZ_TO_MHZ(m) ((m) / ONE_MHZ)

#if defined(SUPPORT_OMAP3430_SGXFCLK_96M)
#define SGX_PARENT_CLOCK "cm_96m_fck"
#else
#define SGX_PARENT_CLOCK "core_ck"
#endif

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
extern struct platform_device *gpsPVRLDMDev;
#endif

static PVRSRV_ERROR PowerLockWrap(SYS_SPECIFIC_DATA *psSysSpecData, IMG_BOOL bTryLock)
{
	if (!in_interrupt())
	{
		if (bTryLock)
		{
			int locked = mutex_trylock(&psSysSpecData->sPowerLock);
			if (locked == 0)
			{
				return PVRSRV_ERROR_RETRY;
			}
		}
		else
		{
			mutex_lock(&psSysSpecData->sPowerLock);
		}
	}

	return PVRSRV_OK;
}

static IMG_VOID PowerLockUnwrap(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (!in_interrupt())
	{
		mutex_unlock(&psSysSpecData->sPowerLock);
	}
}

PVRSRV_ERROR SysPowerLockWrap(IMG_BOOL bTryLock)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	return PowerLockWrap(psSysData->pvSysSpecificData, bTryLock);
}

IMG_VOID SysPowerLockUnwrap(IMG_VOID)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	PowerLockUnwrap(psSysData->pvSysSpecificData);
}

/*
 * This function should be called to unwrap the Services power lock, prior
 * to calling any function that might sleep.
 * This function shouldn't be called prior to calling EnableSystemClocks
 * or DisableSystemClocks, as those functions perform their own power lock
 * unwrapping.
 * If the function returns IMG_TRUE, UnwrapSystemPowerChange must be
 * called to rewrap the power lock, prior to returning to Services.
 */
IMG_BOOL WrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
	return IMG_TRUE;
}

IMG_VOID UnwrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
}

/*
 * Return SGX timining information to caller.
 */
IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psTimingInfo)
{
#if !defined(NO_HARDWARE)
	PVR_ASSERT(atomic_read(&gpsSysSpecificData->sSGXClocksEnabled) != 0);

	psTimingInfo->ui32CoreClockSpeed = GPU_Coreclk_Get();
#else
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
#endif

	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif /* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
}

/*!
******************************************************************************

 @Function  EnableSGXClocks

 @Description Enable SGX clocks

 @Return   PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	/* SGX clocks already enabled? */
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
	{
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));

    owl_gpu_clk_enable();
	//if (gpu_powergate_power_on())
	//	return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	{
		/*
		 * pm_runtime_get_sync returns 1 after the module has
		 * been reloaded.
		 */
		int res = pm_runtime_get_sync(&gpsPVRLDMDev->dev);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: pm_runtime_get_sync failed (%d)", -res));
			return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;
		}
	}
#endif /* defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI) */

	SysEnableSGXInterrupts(psSysData);

	/* Indicate that the SGX clocks are enabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */
	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function  DisableSGXClocks

 @Description Disable SGX clocks.

 @Return   none

******************************************************************************/
IMG_VOID DisableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	/* SGX clocks already disabled? */
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
	{
		return;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));

	SysDisableSGXInterrupts(psSysData);

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	{
		int res = pm_runtime_put_sync(&gpsPVRLDMDev->dev);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "DisableSGXClocks: pm_runtime_put_sync failed (%d)", -res));
		}
	}
#endif /* defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI) */

	owl_gpu_clk_disable();

	/* Indicate that the SGX clocks are disabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */
}

static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecData);

	return PVRSRV_OK;
}
static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecData);
}

/*!
******************************************************************************

 @Function  EnableSystemClocks

 @Description Setup up the clocks for the graphics device to work.

 @Return   PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("EnableSystemClocks: Enabling System Clocks"));
	PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks"));
	if (!psSysSpecData->bSysClocksOneTimeInit)
	{
#if !defined(NO_HARDWARE)
		/* Initial gpu clock. */
		if (gpu_clock_init(&gpsPVRLDMDev->dev))
			return PVRSRV_ERROR_INVALID_DEVICE;
#endif

		mutex_init(&psSysSpecData->sPowerLock);

		atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

		psSysSpecData->bSysClocksOneTimeInit = IMG_TRUE;
	}

	return AcquireGPTimer(psSysSpecData);
}

/*!
******************************************************************************

 @Function  DisableSystemClocks

 @Description Disable the graphics clocks.

 @Return  none

******************************************************************************/
IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("DisableSystemClocks: Disabling System Clocks"));

	/*
	 * Always disable the SGX clocks when the system clocks are disabled.
	 * This saves having to make an explicit call to DisableSGXClocks if
	 * active power management is enabled.
	 */
	DisableSGXClocks(psSysData);

	ReleaseGPTimer(psSysSpecData);
}

PVRSRV_ERROR SysPMRuntimeRegister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	pm_runtime_enable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR SysPMRuntimeUnregister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	pm_runtime_disable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsInitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);
	//gpu_dvfs_init(&(g_gpu_dvfs_data));

	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsDeinitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);

	return PVRSRV_OK;
}

#if defined(SUPPORT_DRI_DRM_PLUGIN)
static struct omap_gpu_plugin sOMAPGPUPlugin;

#define	SYS_DRM_SET_PLUGIN_FIELD(d, s, f) (d)->f = (s)->f
int
SysDRMRegisterPlugin(PVRSRV_DRM_PLUGIN *psDRMPlugin)
{
	int iRes;

	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, name);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, open);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, load);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, unload);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, release);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, mmap);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, ioctls);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, num_ioctls);
	SYS_DRM_SET_PLUGIN_FIELD(&sOMAPGPUPlugin, psDRMPlugin, ioctl_start);

	iRes = omap_gpu_register_plugin(&sOMAPGPUPlugin);
	if (iRes != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: omap_gpu_register_plugin failed (%d)", __FUNCTION__, iRes));
	}

	return iRes;
}

void
SysDRMUnregisterPlugin(PVRSRV_DRM_PLUGIN *psDRMPlugin)
{
	int iRes = omap_gpu_unregister_plugin(&sOMAPGPUPlugin);
	if (iRes != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: omap_gpu_unregister_plugin failed (%d)", __FUNCTION__, iRes));
	}
}
#endif
