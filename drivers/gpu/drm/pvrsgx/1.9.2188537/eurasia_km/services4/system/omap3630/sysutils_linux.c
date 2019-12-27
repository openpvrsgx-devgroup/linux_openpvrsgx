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

#include "sgxdefs.h"
#include "services_headers.h"
#include "sysinfo.h"
#include "sgxapi_km.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

//#include <linux/platform_device.h>
//#include <linux/pm_runtime.h>

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
        SYS_DATA        *psSysData;

        SysAcquireData(&psSysData);

        return PowerLockWrap(psSysData->pvSysSpecificData, bTryLock);
}

IMG_VOID SysPowerLockUnwrap(IMG_VOID)
{
        SYS_DATA        *psSysData;

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

static inline IMG_UINT32 scale_by_rate(IMG_UINT32 val, IMG_UINT32 rate1, IMG_UINT32 rate2)
{
	if (rate1 >= rate2)
	{
		return val * (rate1 / rate2);
	}

	return val / (rate2 / rate1);
}

static inline IMG_UINT32 scale_prop_to_SGX_clock(IMG_UINT32 val, IMG_UINT32 rate)
{
	return scale_by_rate(val, rate, SYS_SGX_CLOCK_SPEED);
}

static inline IMG_UINT32 scale_inv_prop_to_SGX_clock(IMG_UINT32 val, IMG_UINT32 rate)
{
	return scale_by_rate(val, SYS_SGX_CLOCK_SPEED, rate);
}

IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psTimingInfo)
{
	IMG_UINT32 rate;

	rate = SYS_SGX_CLOCK_SPEED;
#if !defined(NO_HARDWARE)
	PVR_ASSERT(atomic_read(&gpsSysSpecificData->sSGXClocksEnabled) != 0);
#endif
	psTimingInfo->ui32CoreClockSpeed = rate;
	psTimingInfo->ui32HWRecoveryFreq = scale_prop_to_SGX_clock(SYS_SGX_HWRECOVERY_TIMEOUT_FREQ, rate);
	psTimingInfo->ui32uKernelFreq = scale_prop_to_SGX_clock(SYS_SGX_PDS_TIMER_FREQ, rate);
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif 
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
}

PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	IMG_INT res;
	long lRate,lNewRate;
	
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
	{
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));
	
	res=clk_enable(psSysSpecData->psSGX_FCK);
	if (res < 0)
        {
                PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Couldn't enable SGX functional clock (%d)", res));
                return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;
        }

	res=clk_enable(psSysSpecData->psSGX_ICK);
        if (res < 0)
        {
                PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Couldn't enable SGX interface clock (%d)", res));

                clk_disable(psSysSpecData->psSGX_FCK);
                return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;
        }

	lNewRate = clk_round_rate(psSysSpecData->psSGX_FCK, SYS_SGX_CLOCK_SPEED + ONE_MHZ);
        if (lNewRate <= 0)
        {
                PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Couldn't round SGX functional clock rate"));
                return PVRSRV_ERROR_UNABLE_TO_ROUND_CLOCK_RATE;
        }


        lRate = clk_get_rate(psSysSpecData->psSGX_FCK);
        if (lRate != lNewRate)
        {
                res = clk_set_rate(psSysSpecData->psSGX_FCK, lNewRate);
                if (res < 0)
                {
                        PVR_DPF((PVR_DBG_WARNING, "EnableSGXClocks: Couldn't set SGX functional clock rate (%d)", res));
			return PVRSRV_ERROR_UNABLE_TO_SET_CLOCK_RATE;
                }
        }

#if defined(DEBUG)
        {
                IMG_UINT32 rate = clk_get_rate(psSysSpecData->psSGX_FCK);
                PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: SGX Functional Clock is %dMhz", HZ_TO_MHZ(rate)));
        }
#endif



#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	{
		
//		int res = pm_runtime_get_sync(&gpsPVRLDMDev->dev);
//		if (res < 0)
//		{
//			PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: pm_runtime_get_sync failed (%d)", -res));
//			return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;
//		}
	}
#endif
//	SysEnableSGXInterrupts(psSysData);

	
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

#else	
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	
	return PVRSRV_OK;
}


IMG_VOID DisableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
	{
		return;
	}
	
	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));
	
	clk_disable(psSysSpecData->psSGX_FCK);

	clk_disable(psSysSpecData->psSGX_ICK);

//	SysDisableSGXInterrupts(psSysData);

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	{
//		int res = pm_runtime_put_sync(&gpsPVRLDMDev->dev);
//		if (res < 0)
//		{
//			PVR_DPF((PVR_DBG_ERROR, "DisableSGXClocks: pm_runtime_put_sync failed (%d)", -res));
//		}
	}
#endif

	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

#else	
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	
}

#if (defined(DEBUG) || defined(TIMING)) && !defined(PVR_NO_OMAP_TIMER)
#if defined(PVR_OMAP_USE_DM_TIMER_API)
#define	GPTIMER_TO_USE 11
static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_ASSERT(psSysSpecData->psGPTimer == NULL);

	
	psSysSpecData->psGPTimer = omap_dm_timer_request_specific(GPTIMER_TO_USE);
	if (psSysSpecData->psGPTimer == NULL)
	{
	
		PVR_DPF((PVR_DBG_WARNING, "%s: omap_dm_timer_request_specific failed", __FUNCTION__));
		return PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
	}

	
	omap_dm_timer_set_source(psSysSpecData->psGPTimer, OMAP_TIMER_SRC_SYS_CLK);
	omap_dm_timer_enable(psSysSpecData->psGPTimer);

	
	omap_dm_timer_set_load_start(psSysSpecData->psGPTimer, 1, 0);

	omap_dm_timer_start(psSysSpecData->psGPTimer);

	
	psSysSpecData->sTimerRegPhysBase.uiAddr = SYS_OMAP3430_GP11TIMER_REGS_SYS_PHYS_BASE;

	return PVRSRV_OK;
}

static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (psSysSpecData->psGPTimer != NULL)
	{
			
		(void) omap_dm_timer_stop(psSysSpecData->psGPTimer);

		omap_dm_timer_disable(psSysSpecData->psGPTimer);

		omap_dm_timer_free(psSysSpecData->psGPTimer);

		psSysSpecData->sTimerRegPhysBase.uiAddr = 0;

		psSysSpecData->psGPTimer = NULL;
	}

}
#else	
static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
#if defined(PVR_OMAP3_TIMING_PRCM)
	struct clk *psCLK;
	IMG_INT res;
	struct clk *sys_ck;
	IMG_INT rate;
#endif
	PVRSRV_ERROR eError;

	IMG_CPU_PHYADDR sTimerRegPhysBase;
	IMG_HANDLE hTimerEnable;
	IMG_UINT32 *pui32TimerEnable;
#if defined(PVR_OMAP_TIMER_BASE_IN_SYS_SPEC_DATA)
	PVR_ASSERT(psSysSpecData->sTimerRegPhysBase.uiAddr == 0);
#endif


#if defined(PVR_OMAP3_TIMING_PRCM)
	
	psCLK = clk_get(NULL, "gpt11_fck");
	if (IS_ERR(psCLK))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 functional clock"));
		goto ExitError;
	}
	psSysSpecData->psGPT11_FCK = psCLK;

	psCLK = clk_get(NULL, "gpt11_ick");
	if (IS_ERR(psCLK))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 interface clock"));
		goto ExitError;
	}
	psSysSpecData->psGPT11_ICK = psCLK;

	sys_ck = clk_get(NULL, "sys_ck");
	if (IS_ERR(sys_ck))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get System clock"));
		goto ExitError;
	}

	if(clk_get_parent(psSysSpecData->psGPT11_FCK) != sys_ck)
	{
		PVR_TRACE(("Setting GPTIMER11 parent to System Clock"));
		res = clk_set_parent(psSysSpecData->psGPT11_FCK, sys_ck);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't set GPTIMER11 parent clock (%d)", res));
		goto ExitError;
		}
	}
	rate = clk_get_rate(psSysSpecData->psGPT11_FCK);
	PVR_TRACE(("GPTIMER11 clock is %dMHz", HZ_TO_MHZ(rate)));

	res = clk_enable(psSysSpecData->psGPT11_FCK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't enable GPTIMER11 functional clock (%d)", res));
		goto ExitError;
	}

	res = clk_enable(psSysSpecData->psGPT11_ICK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't enable GPTIMER11 interface clock (%d)", res));
		goto ExitDisableGPT11FCK;
	}
#endif	

	
	sTimerRegPhysBase.uiAddr = SYS_OMAP3430_GP11TIMER_TSICR_SYS_PHYS_BASE;
	pui32TimerEnable = OSMapPhysToLin(sTimerRegPhysBase,
                  4,
                  PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
                  &hTimerEnable);

	if (pui32TimerEnable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: OSMapPhysToLin failed"));
		goto ExitDisableGPT11ICK;
	}

	if(!(*pui32TimerEnable & 4))
	{
		PVR_TRACE(("Setting GPTIMER11 mode to posted (currently is non-posted)"));

		
		*pui32TimerEnable |= 4;
	}

	OSUnMapPhysToLin(pui32TimerEnable,
		    4,
		    PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
		    hTimerEnable);

	
	sTimerRegPhysBase.uiAddr = SYS_OMAP3430_GP11TIMER_ENABLE_SYS_PHYS_BASE;
	pui32TimerEnable = OSMapPhysToLin(sTimerRegPhysBase,
                  4,
                  PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
                  &hTimerEnable);

	if (pui32TimerEnable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: OSMapPhysToLin failed"));
		goto ExitDisableGPT11ICK;
	}

	*pui32TimerEnable = 3;

	OSUnMapPhysToLin(pui32TimerEnable,
		    4,
		    PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
		    hTimerEnable);
#if defined(PVR_OMAP_TIMER_BASE_IN_SYS_SPEC_DATA)
	psSysSpecData->sTimerRegPhysBase = sTimerRegPhysBase;
#endif

	eError = PVRSRV_OK;

	goto Exit;

ExitDisableGPT11ICK:
#if defined(PVR_OMAP3_TIMING_PRCM)
	clk_disable(psSysSpecData->psGPT11_ICK);
ExitDisableGPT11FCK:
	clk_disable(psSysSpecData->psGPT11_FCK);
ExitError:
#endif	
	eError = PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
Exit:
	return eError;
}

static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	IMG_HANDLE hTimerDisable;
	IMG_UINT32 *pui32TimerDisable;
	IMG_CPU_PHYADDR TimerRegPhysBase;
#if defined(PVR_OMAP_TIMER_BASE_IN_SYS_SPEC_DATA)
	if (psSysSpecData->sTimerRegPhysBase.uiAddr == 0)
	{
		return;
	}
#endif
	TimerRegPhysBase.uiAddr = SYS_OMAP3430_GP11TIMER_ENABLE_SYS_PHYS_BASE;	
	pui32TimerDisable = OSMapPhysToLin(TimerRegPhysBase,
				4,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				&hTimerDisable);

	if (pui32TimerDisable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "DisableSystemClocks: OSMapPhysToLin failed"));
	}
	else
	{
		*pui32TimerDisable = 0;

		OSUnMapPhysToLin(pui32TimerDisable,
				4,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				hTimerDisable);
	}

#if defined(PVR_OMAP_TIMER_BASE_IN_SYS_SPEC_DATA)
	psSysSpecData->sTimerRegPhysBase.uiAddr = 0;
#endif

#if defined(PVR_OMAP3_TIMING_PRCM)
	clk_disable(psSysSpecData->psGPT11_ICK);

	clk_disable(psSysSpecData->psGPT11_FCK);
#endif	
}
#endif	
#else	
static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecData);
	return PVRSRV_OK;
}
static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecData);
}
#endif 

PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	struct clk *psCLK;
	IMG_INT res;
	PVR_TRACE(("EnableSystemClocks: Enabling System Clocks"));

	if (!psSysSpecData->bSysClocksOneTimeInit)
	{
		mutex_init(&psSysSpecData->sPowerLock);

		atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

                psCLK = clk_get(NULL, SGX_PARENT_CLOCK);
                if (IS_ERR(psCLK))
                {
                        PVR_DPF((PVR_DBG_ERROR, "EnableSsystemClocks: Couldn't get Core Clock"));
                        return PVRSRV_ERROR_UNABLE_TO_GET_PARENT_CLOCK;
                }
                psSysSpecData->psCORE_CK = psCLK;

		psCLK = clk_get(NULL, "sgx_fck");
                if (IS_ERR(psCLK))
                {
                        PVR_DPF((PVR_DBG_ERROR, "EnableSsystemClocks: Couldn't get SGX Functional Clock"));
                        return PVRSRV_ERROR_UNABLE_TO_GET_CLOCK;
                }
                psSysSpecData->psSGX_FCK = psCLK;

                psCLK = clk_get(NULL, "sgx_ick");
                if (IS_ERR(psCLK))
                {
                        PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get SGX Interface Clock"));
                        return PVRSRV_ERROR_UNABLE_TO_GET_CLOCK;
                }
                psSysSpecData->psSGX_ICK = psCLK;
		
		res = clk_set_parent(psSysSpecData->psSGX_FCK, psSysSpecData->psCORE_CK);
                if (res < 0)
                {
                        PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't set SGX parent clock (%d)", res));
                        return PVRSRV_ERROR_UNABLE_TO_GET_PARENT_CLOCK;
                }


		psSysSpecData->bSysClocksOneTimeInit = IMG_TRUE;
	}

	return AcquireGPTimer(psSysSpecData);
}

IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	PVR_TRACE(("DisableSystemClocks: Disabling System Clocks"));
	
	DisableSGXClocks(psSysData);

	ReleaseGPTimer(psSysSpecData);

}

PVRSRV_ERROR SysPMRuntimeRegister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
//	pm_runtime_enable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR SysPMRuntimeUnregister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
//	pm_runtime_disable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}
