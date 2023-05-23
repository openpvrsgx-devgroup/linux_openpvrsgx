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
#include <aw/platform.h>
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "sysinfo.h"
#include "sgxapi_km.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <mach/hardware.h>
#include <mach/platform.h>
//#include <mach/clock.h>

#include "oemfuncs.h"

#define	ONE_MHZ	1000000
#define	HZ_TO_MHZ(m) ((m) / ONE_MHZ)

struct clk *h_ahb_gpu, *h_gpu_coreclk, *h_gpu_hydclk, *h_gpu_memclk, *h_gpu_hydpll, *h_gpu_corepll;
struct regulator *gpu_power;

#if defined(LDM_PLATFORM) 
extern struct platform_device *gpsPVRLDMDev;
#endif

extern int ths_read_data(int value);

extern unsigned int sunxi_get_soc_bin(void);

static long GetTemperature(IMG_VOID)
{
    return ths_read_data(private_data.sensor_num);
}

static IMG_VOID AssertResetSignal(struct aw_clk_data clk_data)
{
	if(clk_data.need_reset)
	{
		if(sunxi_periph_reset_assert(clk_data.clk_handle))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to pull down gpu %s clock reset!", clk_data.clk_name));
		}
	}
}

static IMG_VOID DeAssertResetSignal(struct aw_clk_data clk_data)
{
	if(clk_data.need_reset)
	{
		if(sunxi_periph_reset_deassert(clk_data.clk_handle))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to release gpu %s clock reset!", clk_data.clk_name));
		}
	}
}

static IMG_VOID AssertGpuResetSignal(IMG_VOID)
{
	int i;
	for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
	{
		AssertResetSignal(clk_data[i]);
	}
}

static IMG_VOID DeAssertGpuResetSignal(IMG_VOID)
{
	int i;
	for(i = sizeof(clk_data)/sizeof(clk_data[0]) - 1; i >= 0; i--)
	{
		DeAssertResetSignal(clk_data[i]);
	}
}

static IMG_VOID EnableGpuPower(IMG_VOID)
{
	if(!regulator_is_enabled(private_data.regulator))
	{
		if(regulator_enable(private_data.regulator))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu external power!"));
		}
	}
}

static IMG_VOID DisableGpuPower(IMG_VOID)
{
	if(regulator_is_enabled(private_data.regulator))
	{
		if (regulator_disable(private_data.regulator))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to disable gpu external power!"));
		}
	}
}

static int GetCurrentVol(IMG_VOID)
{
	return regulator_get_voltage(private_data.regulator)/1000;
}

static IMG_VOID SetGpuVol(int vol /* mV */)
{
	if(vol <= 1300)
	{
		if(vol != GetCurrentVol())
		{
			if(regulator_set_voltage(private_data.regulator, vol*1000, vol*1000) != 0)
			{
				PVR_DPF((PVR_DBG_ERROR, "Failed to set gpu power voltage: Current voltage is %d mV, the voltage to be is %d mV", GetCurrentVol()/1000, vol));
				return;
			}

			/* Delay for gpu voltage stability */
			udelay(20);
		}
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "Failed to set gpu power voltage: The voltage to be is %d mV, it is beyond the permitted voltage boundary 1300 mV", vol));
	}
}

static long int GetCurrentFreq(IMG_VOID)
{
    return clk_get_rate(clk_data[0].clk_handle)/(1000 * 1000);
}

static IMG_VOID GetGpuClk(IMG_VOID)
{
	int i;
	for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
	{
		clk_data[i].clk_handle = clk_get(NULL, clk_data[i].clk_id);
		if(NULL == clk_data[i].clk_handle)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to get gpu %s clock id!", clk_data[i].clk_name));
			return;
		}
	}
}

static IMG_VOID SetClkVal(int freq /* MHz */)
{	
	int i;

	if(freq != GetCurrentFreq())
	{
		for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
		{
			if(clk_data[i].expected_freq == 0)
			{
				continue;
			}
			else if(clk_data[i].expected_freq > 1)
			{
				freq = clk_data[i].expected_freq;
			}

			if(clk_set_rate(clk_data[i].clk_handle, freq*1000*1000))
			{
				PVR_DPF((PVR_DBG_ERROR, "Failed to set the frequency of gpu %s clock: Current frequency is %ld MHz, the frequency to be is %d MHz", clk_data[i].clk_name, GetCurrentFreq(), freq));
			}
		}
	}
}

static IMG_VOID SetGpuClkVal(int freq /* MHz */)
{
	PVRSRV_ERROR err;

	if(freq == GetCurrentFreq())
	{
		return;
	}

	mutex_lock(&private_data.dvfs_lock);
	err = PVRSRVDevicePreClockSpeedChange(0, IMG_TRUE, NULL);
	if(err == PVRSRV_OK)
	{
		SetClkVal(freq);
		PVRSRVDevicePostClockSpeedChange(0, IMG_TRUE, NULL);
	}
	mutex_unlock(&private_data.dvfs_lock);
}

static IMG_VOID DvfsChange(int freq /* MHz */)
{
	PVRSRV_ERROR err;

	if(freq == GetCurrentFreq())
	{
		return;
	}

	mutex_lock(&private_data.dvfs_lock);
	err = PVRSRVDevicePreClockSpeedChange(0, IMG_TRUE, NULL);
	if(err == PVRSRV_OK)
	{
		if(GetCurrentFreq() <= vf_data.normal.freq)
		{
			if(freq <= vf_data.normal.freq)
			{
				if(GetCurrentVol() != vf_data.normal.vol)
				{
					SetGpuVol(vf_data.normal.vol);
				}
			}
			else
			{
				if(GetCurrentVol() != vf_data.extreme.vol)
				{
					SetGpuVol(vf_data.extreme.vol);
				}
			}
			SetClkVal(freq);
		}
		else
		{
			if(freq <= vf_data.normal.freq)
			{
				SetClkVal(freq);
				if(GetCurrentVol() != vf_data.normal.vol)
				{
					SetGpuVol(vf_data.normal.vol);
				}
			}
			else
			{
				if(GetCurrentVol() != vf_data.extreme.vol)
				{
					SetGpuVol(vf_data.extreme.vol);
				}
				SetClkVal(freq);
			}
		}

		PVRSRVDevicePostClockSpeedChange(0, IMG_TRUE, NULL);
	}
	mutex_unlock(&private_data.dvfs_lock);
}

static IMG_VOID SetGpuClkParent(IMG_VOID)
{
	int i, j;
	for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
	{
		if(NULL != clk_data[i].clk_parent_id)
		{
			for(j = 0; j < sizeof(clk_data)/sizeof(clk_data[0]); j++)
			{
				if(clk_data[i].clk_parent_id == clk_data[j].clk_id)
				{
					if (clk_set_parent(clk_data[i].clk_handle, clk_data[j].clk_handle))
					{
						PVR_DPF((PVR_DBG_ERROR, "Failed to set the parent of gpu %s clock!", clk_data[i].clk_name));
					}
				}
			}
		}
	}
}

static IMG_VOID SetBit(unsigned char bit, unsigned int val, void *addr)
{
	unsigned char reg_data;
	reg_data = readl(addr);
	if(val == 0)
	{
		reg_data &= ~(1 << bit);
		sunxi_smc_writel(reg_data, addr);
	}
	else if(val == 1)
	{
		reg_data |= (1 << bit);
		sunxi_smc_writel(reg_data, addr);
	}
}

#ifdef CONFIG_CPU_BUDGET_THERMAL
static int gpu_throttle_notifier_call(struct notifier_block *nfb, unsigned long mode, void *cmd)
{
    int retval = NOTIFY_DONE, i = 0;

    if(private_data.tempctrl_data.temp_ctrl_status == 0)
    {
		long temperature = GetTemperature();
		if(temperature > tf_table[0].temp)
        {
            for(i = private_data.tempctrl_data.count - 1; i >= 0; i--)
            {
                if(temperature >= tf_table[i].temp)
                {
					DvfsChange(tf_table[i].freq);
                    private_data.max_freq = tf_table[i].freq;
                    break;
                }
            }
        }
    }

    return retval;
}

static struct notifier_block gpu_throttle_notifier = {
.notifier_call = gpu_throttle_notifier_call,
};
#endif /* CONFIG_CPU_BUDGET_THERMAL */
 
static ssize_t AndroidFreqValueShow(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%ld MHz\n", GetCurrentFreq());
}

static ssize_t AndroidFreqValueStore(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long cmd;	
	
	if(private_data.scene_ctrl_status)
	{
		err = strict_strtoul(buf, 10, &cmd);
		if(err)
		{
			PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
			goto out;
		}

		if(cmd == 0)
		{
			if(private_data.tempctrl_data.temp_ctrl_status && vf_data.normal.freq > private_data.max_freq)
			{
				goto out;
			}
			/* Recover to normal frequency */
			else
            {
                DvfsChange(vf_data.normal.freq);
            }
		}
		else if(cmd == 1)
		{
			if(private_data.tempctrl_data.temp_ctrl_status && vf_data.extreme.freq > private_data.max_freq)
			{
				goto out;
			}
			else
            {
                /* Run in extreme mode */
				DvfsChange(vf_data.extreme.freq);
            }
		}
	}

out:
	return count;
}

static ssize_t ManualFreqValueShow(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%ld MHz\n", GetCurrentFreq());
}

static ssize_t ManualFreqValueStore(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long freq;
    err = strict_strtoul(buf, 10, &freq);

    if(err)
    {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
		goto err_out;
	}

	SetGpuClkVal(freq);

err_out:
	return count;
}

static ssize_t SceneCtrlShow(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", private_data.scene_ctrl_status);
}

static ssize_t SceneCtrlStore(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long tmp;
	err = strict_strtoul(buf, 10, &tmp);
	if(err)
    {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
		goto out;
	}

	if(tmp == 0 || tmp == 1)
	{
		private_data.scene_ctrl_status = tmp;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "The number is too large!"));
	}

out:	
	return count;
}

static ssize_t TempCtrlShow(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "sensor: %d, status: %d, temperature: %ld\n", private_data.sensor_num, private_data.tempctrl_data.temp_ctrl_status, GetTemperature());
}

static ssize_t TempCtrlStore(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long tmp;
	err = strict_strtoul(buf, 10, &tmp);
	if(err)
    {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
		goto out;
	}

	if(tmp == 0 || tmp == 1)
	{
		private_data.tempctrl_data.temp_ctrl_status = tmp;
	}

out:	
	return count;
}

static ssize_t VoltageShow(struct device *dev, struct device_attribute *attr, char *buf)
{
        int count = 0;
        if (!IS_ERR_OR_NULL(private_data.regulator))
        {
            count = sprintf(buf, "%d mV\n", GetCurrentVol());
        }
        return count;
}

static ssize_t VoltageStore(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	PVRSRV_ERROR err;
    unsigned long vol;
    err = strict_strtoul(buf, 10, &vol);
    if (err)
    {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!\n"));
        goto out;
    }

	mutex_lock(&private_data.dvfs_lock);
	err = PVRSRVDevicePreClockSpeedChange(0, IMG_TRUE, NULL);
	if(err == PVRSRV_OK)
	{
		SetGpuVol(vol);

		PVRSRVDevicePostClockSpeedChange(0, IMG_TRUE, NULL);
	}
	mutex_unlock(&private_data.dvfs_lock);

out:
    return count;
}

static DEVICE_ATTR(android, S_IRUGO|S_IWUSR|S_IWGRP, AndroidFreqValueShow, AndroidFreqValueStore);

static DEVICE_ATTR(manual, S_IRUGO|S_IWUSR|S_IWGRP, ManualFreqValueShow, ManualFreqValueStore);

static DEVICE_ATTR(scenectrl, S_IRUGO|S_IWUSR|S_IWGRP, SceneCtrlShow, SceneCtrlStore);

static DEVICE_ATTR(tempctrl, S_IRUGO|S_IWUSR|S_IWGRP, TempCtrlShow, TempCtrlStore);

static DEVICE_ATTR(voltage, S_IRUGO|S_IWUSR|S_IWGRP, VoltageShow, VoltageStore);

static struct attribute *gpu_attributes[] =
{
	&dev_attr_android.attr,
    &dev_attr_manual.attr,
    &dev_attr_scenectrl.attr,
	&dev_attr_tempctrl.attr,
	&dev_attr_voltage.attr,
    NULL
};

 struct attribute_group gpu_attribute_group = {
  .name = "dvfs",
  .attrs = gpu_attributes
};

static int GetParaFromFex(char *main_key, char *second_key, int max_value)
{
    u32 value;
    script_item_u val;
    script_item_value_type_e type;
    type = script_get_item(main_key, second_key, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
	{
        PVR_DPF((PVR_DBG_ERROR, "%s: %s in sys_config.fex is invalid!\n", main_key, second_key));
        return -1;
    }
    value = val.val;

    if(max_value)
    {
        if(value <= max_value)
        {
            return value;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return value;
    }
}

static IMG_VOID ParseFex(IMG_VOID)
{
	int value;
    int i;
	char mainkey[9] = {0};
    char tfx_name[10] = {0};

	sprintf(mainkey, "%s%d", "gpu_sgx544_", sunxi_get_soc_bin());

	value = GetParaFromFex(mainkey, "normal_vol", 0);
    if(value > 0)
    {
        vf_data.normal.vol = value;
    }

    value = GetParaFromFex(mainkey, "normal_freq", 0);
    if(value > 0)
    {
        vf_data.normal.freq = value;
    }

	value = GetParaFromFex(mainkey, "extreme_vol", 0);
    if(value > 0)
    {
        vf_data.extreme.vol = value;
    }

    value = GetParaFromFex(mainkey, "extreme_freq", 0);
    if(value > 0)
    {
        vf_data.extreme.freq = value;
    }

    value = GetParaFromFex(mainkey, "scene_ctrl_status", 1);
    if(value != -1)
    {
        private_data.scene_ctrl_status = value;
    }

    value = GetParaFromFex(mainkey, "temp_ctrl_status", 1);
    if(value != -1)
    {
        private_data.tempctrl_data.temp_ctrl_status = value;
    }

#ifdef CONFIG_CPU_BUDGET_THERMAL
    value = GetParaFromFex(mainkey, "tft_count", sizeof(tf_table)/sizeof(tf_table[0]));
    if(value > 0)
    {
		if(value < private_data.tempctrl_data.count)
		{
			private_data.tempctrl_data.count = value;
		}

        for(i = 0; i < private_data.tempctrl_data.count; i++)
        {
            sprintf(tfx_name, "tf%d_temp",i);
            value = GetParaFromFex(mainkey, tfx_name, 0);
            if(value != -1)
            {
                tf_table[i].temp = value;
            }

            sprintf(tfx_name, "tf%d_freq",i);
            value = GetParaFromFex(mainkey, tfx_name, 0);
            if(value != -1)
            {
                tf_table[i].freq = value;
            }
        }
	}
#endif /* CONFIG_CPU_BUDGET_THERMAL */
}
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
#endif
	psTimingInfo->ui32CoreClockSpeed = (IMG_UINT32)GetCurrentFreq() * 1000 * 1000;
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
	int i;
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	/* SGX clocks already enabled? */
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
	{
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));

	/* Enable PLL and gpu related clocks */
	for(i = 0; i < sizeof(clk_data)/sizeof(clk_data[0]); i++)
	{
		if(clk_prepare_enable(clk_data[i].clk_handle))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu %s clock!\n", clk_data[i].clk_name));
		}
	}
	
#if defined(LDM_PLATFORM)
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
#endif /* defined(LDM_PLATFORM)*/

	SysEnableSGXInterrupts(psSysData);

	/* Indicate that the SGX clocks are enabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */

        AWDEBUG("%s: sgx clock has been enabled ",__func__);
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
	int i;
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	/* SGX clocks already disabled? */
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
	{
		return;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));

	SysDisableSGXInterrupts(psSysData);

	for(i = sizeof(clk_data)/sizeof(clk_data[0]) - 1; i >= 0; i--)
	{
		clk_disable_unprepare(clk_data[i].clk_handle);
	}
	
	
#if defined(LDM_PLATFORM)
	{
		int res = pm_runtime_put_sync(&gpsPVRLDMDev->dev);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "DisableSGXClocks: pm_runtime_put_sync failed (%d)", -res));
		}
	}
#endif /* defined(LDM_PLATFORM)*/

	/* Indicate that the SGX clocks are disabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */
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
	if (!psSysSpecData->bSysClocksOneTimeInit)
	{
		if(NULL != private_data.regulator_id)
		{
			private_data.regulator = regulator_get(NULL,private_data.regulator_id);
			if (IS_ERR(private_data.regulator))
			{
				PVR_DPF((PVR_DBG_ERROR, "Failed to get gpu regulator!"));
			}
		}

		if(regulator_enable(private_data.regulator))
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu external power!"));
		}

#ifdef CONFIG_CPU_BUDGET_THERMAL
        private_data.tempctrl_data.count = sizeof(tf_table)/sizeof(tf_table[0]);
#endif /* CONFIG_CPU_BUDGET_THERMAL */

		ParseFex();

		private_data.max_freq = vf_data.extreme.freq;

		GetGpuClk();

		SetGpuClkParent();

		SetGpuVol(vf_data.normal.vol);

		SetClkVal(vf_data.normal.freq);

		mutex_init(&psSysSpecData->sPowerLock);

		atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

		psSysSpecData->bSysClocksOneTimeInit = IMG_TRUE;

		mutex_init(&private_data.dvfs_lock);

		sysfs_create_group(&gpsPVRLDMDev->dev.kobj, &gpu_attribute_group);

	#ifdef CONFIG_CPU_BUDGET_THERMAL
        register_budget_cooling_notifier(&gpu_throttle_notifier);
    #endif /* CONFIG_CPU_BUDGET_THERMAL */
	}

	EnableGpuPower();
	
	/* Delay for gpu power stability */
	mdelay(2);

	/* Set gpu power off gating invalid */
	SetBit(private_data.poweroff_gate.bit, 0, private_data.poweroff_gate.addr);

	DeAssertGpuResetSignal();

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function  DisableSystemClocks

 @Description Disable the graphics clocks.

 @Return  none

******************************************************************************/
IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	PVR_TRACE(("DisableSystemClocks: Disabling System Clocks"));

	AssertGpuResetSignal();

	/*
	 * Always disable the SGX clocks when the system clocks are disabled.
	 * This saves having to make an explicit call to DisableSGXClocks if
	 * active power management is enabled.
	 */
	DisableSGXClocks(psSysData);
	
	/* Set gpu power off gating valid */
	SetBit(private_data.poweroff_gate.bit, 1, private_data.poweroff_gate.addr);

	/* Disable gpu power */
	DisableGpuPower();
}

PVRSRV_ERROR SysPMRuntimeRegister(void)
{
	pm_runtime_enable(&gpsPVRLDMDev->dev);

	return PVRSRV_OK;
}

PVRSRV_ERROR SysPMRuntimeUnregister(void)
{
	pm_runtime_disable(&gpsPVRLDMDev->dev);

	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsInitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);

	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsDeinitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);

	return PVRSRV_OK;
}

