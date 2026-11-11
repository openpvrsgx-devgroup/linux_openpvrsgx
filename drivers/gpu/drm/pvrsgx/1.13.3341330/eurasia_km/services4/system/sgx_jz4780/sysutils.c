/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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

#include <linux/version.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "sysinfo.h"
#include "sgxapi_km.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#if defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
#include <linux/opp.h>
#endif

#if defined(SUPPORT_DRI_DRM_PLUGIN)
#include <drm/drmP.h>
#include <drm/drm.h>

#include <linux/xb_gpu.h>

#include "pvr_drm.h"
#endif

#include <mach/jzcpm_pwc.h>

/* Defines for HW Recovery */
#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100) // 10ms (100hz)
#define SYS_SGX_PDS_TIMER_FREQ				(1000) // 1ms (1000hz)

#define	ONE_MHZ	1000000
#define	HZ_TO_MHZ(m) ((m) / ONE_MHZ)

#define SGX_PARENT_CLOCK "core_ck"

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

IMG_BOOL WrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
	return IMG_TRUE;
}

IMG_VOID UnwrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
}

IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psTimingInfo)
{
	PVR_ASSERT(atomic_read(&gpsSysSpecificData->sSGXClocksEnabled) != 0);
#if defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
	psTimingInfo->ui32CoreClockSpeed =
		gpsSysSpecificData->pui32SGXFreqList[gpsSysSpecificData->ui32SGXFreqListIndex];
#else 
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
#endif
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif 
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
}

PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
		return PVRSRV_OK;

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
#if defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
	{
		struct gpu_platform_data *pdata;
		IMG_UINT32 max_freq_index;
		int res;

		pdata = (struct gpu_platform_data *)gpsPVRLDMDev->dev.platform_data;
		max_freq_index = psSysSpecData->ui32SGXFreqListSize - 2;

		if (psSysSpecData->ui32SGXFreqListIndex != max_freq_index)
		{
			PVR_ASSERT(pdata->device_scale != IMG_NULL);
			res = pdata->device_scale(&gpsPVRLDMDev->dev,
									  &gpsPVRLDMDev->dev,
									  psSysSpecData->pui32SGXFreqList[max_freq_index]);
			if (res == 0)
			{
				psSysSpecData->ui32SGXFreqListIndex = max_freq_index;
			}
			else if (res == -EBUSY)
			{
				PVR_DPF((PVR_DBG_WARNING, "EnableSGXClocks: Unable to scale SGX frequency (EBUSY)"));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
			else if (res < 0)
			{
				PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Unable to scale SGX frequency (%d)", res));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
		}
	}
#endif 
#endif 

	SysEnableSGXInterrupts(psSysData);

	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

	return PVRSRV_OK;
}


IMG_VOID DisableSGXClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
		return;

	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));

	SysDisableSGXInterrupts(psSysData);

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
#if defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
	{
		struct gpu_platform_data *pdata;
		int res;

		pdata = (struct gpu_platform_data *)gpsPVRLDMDev->dev.platform_data;

		
		if (psSysSpecData->ui32SGXFreqListIndex != 0)
		{
			PVR_ASSERT(pdata->device_scale != IMG_NULL);
			res = pdata->device_scale(&gpsPVRLDMDev->dev,
									  &gpsPVRLDMDev->dev,
									  psSysSpecData->pui32SGXFreqList[0]);
			if (res == 0)
			{
				psSysSpecData->ui32SGXFreqListIndex = 0;
			}
			else if (res == -EBUSY)
			{
				PVR_DPF((PVR_DBG_WARNING, "DisableSGXClocks: Unable to scale SGX frequency (EBUSY)"));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
			else if (res < 0)
			{
				PVR_DPF((PVR_DBG_ERROR, "DisableSGXClocks: Unable to scale SGX frequency (%d)", res));
				psSysSpecData->ui32SGXFreqListIndex = psSysSpecData->ui32SGXFreqListSize - 1;
			}
		}
	}
#endif
#endif

	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);
}

PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	int err;

	PVR_TRACE(("EnableSystemClocks: Enabling System Clocks"));

	/* power up */
	err = cpm_pwc_enable(psSysSpecData->pCPMHandle);
	if (err) {
		PVR_DPF((PVR_DBG_ERROR, "%s failed to power up GPU: %d", __func__, err));
		return PVRSRV_ERROR_UNKNOWN_POWER_STATE;
	}

	/* ungate clock */
	err = clk_enable(psSysSpecData->psSGXClockGate);
	if (err) {
		PVR_DPF((PVR_DBG_ERROR, "%s failed to ungate GPU clock: %d", __func__, err));
		return PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
	}

	/* enable clock */
	err = clk_set_rate(psSysSpecData->psSGXClock, SYS_SGX_CLOCK_SPEED);
	if (!err)
		err = clk_enable(psSysSpecData->psSGXClock);
	if (err) {
		PVR_DPF((PVR_DBG_ERROR, "%s failed to initialise GPU clock: %d", __func__, err));
		return PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
	}

	return PVRSRV_OK;
}

IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("DisableSystemClocks: Disabling System Clocks"));

	DisableSGXClocks(psSysData);

	/* disable & gate clock, power down */
	clk_disable(psSysSpecData->psSGXClock);
	clk_disable(psSysSpecData->psSGXClockGate);
	cpm_pwc_disable(psSysSpecData->pCPMHandle);
}

PVRSRV_ERROR SysDvfsInitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
#if !defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);

        // james
        /* clk = clk_get(&gpsPVRLDMDev->dev, SYS_XB4780_GPU_TIMER_NAME); */
	/* if(!clk) { */
        /*     PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not get clock for GPU")); */
        /*     return PVRSRV_ERROR_NOT_SUPPORTED; */
	/* } */
#else 
	IMG_UINT32 i, *freq_list;
	IMG_INT32 opp_count;
	unsigned long freq;
	struct opp *opp;

	
	rcu_read_lock();
	opp_count = opp_get_opp_count(&gpsPVRLDMDev->dev);
	if (opp_count < 1)
	{
		rcu_read_unlock();
		PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not retrieve opp count"));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	
	freq_list = kmalloc((opp_count + 1) * sizeof(IMG_UINT32), GFP_ATOMIC);
	if (!freq_list)
	{
		rcu_read_unlock();
		PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not allocate frequency list"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	
	freq = 0;
	for (i = 0; i < opp_count; i++)
	{
		opp = opp_find_freq_ceil(&gpsPVRLDMDev->dev, &freq);
		if (IS_ERR_OR_NULL(opp))
		{
			rcu_read_unlock();
			PVR_DPF((PVR_DBG_ERROR, "SysDvfsInitialize: Could not retrieve opp level %d", i));
			kfree(freq_list);
			return PVRSRV_ERROR_NOT_SUPPORTED;
		}
		freq_list[i] = (IMG_UINT32)freq;
		freq++;
	}
	rcu_read_unlock();
	freq_list[opp_count] = freq_list[opp_count - 1];

	psSysSpecificData->ui32SGXFreqListSize = opp_count + 1;
	psSysSpecificData->pui32SGXFreqList = freq_list;

	
	psSysSpecificData->ui32SGXFreqListIndex = opp_count;
#endif 

	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsDeinitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
#if !defined(SYS_XB47_HAS_DVFS_FRAMEWORK)
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);
#else 
	if (psSysSpecificData->ui32SGXFreqListIndex != 0)
	{
		struct gpu_platform_data *pdata;
		IMG_INT32 res;

		pdata = (struct gpu_platform_data *)gpsPVRLDMDev->dev.platform_data;

		PVR_ASSERT(pdata->device_scale != IMG_NULL);
		res = pdata->device_scale(&gpsPVRLDMDev->dev,
								  &gpsPVRLDMDev->dev,
								  psSysSpecificData->pui32SGXFreqList[0]);
		if (res == -EBUSY)
		{
			PVR_DPF((PVR_DBG_WARNING, "SysDvfsDeinitialize: Unable to scale SGX frequency (EBUSY)"));
		}
		else if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "SysDvfsDeinitialize: Unable to scale SGX frequency (%d)", res));
		}

		psSysSpecificData->ui32SGXFreqListIndex = 0;
	}

	kfree(psSysSpecificData->pui32SGXFreqList);
	psSysSpecificData->pui32SGXFreqList = 0;
	psSysSpecificData->ui32SGXFreqListSize = 0;
#endif 

	return PVRSRV_OK;
}
