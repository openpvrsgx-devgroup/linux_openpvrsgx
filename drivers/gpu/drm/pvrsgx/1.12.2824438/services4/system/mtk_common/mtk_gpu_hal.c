
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   mtk_gpu_hal.c
 *
 * Project:
 * --------
 *   MT02183
 *
 * Description:
 * ------------
 *   Implementation of mtk gpu hal
 *
 * Author:
 * -------
 *   Nick Huang
 *
 ****************************************************************************/
#include "mtk_gpu_hal.h" 
#include "services.h"
#include "servicesext.h"
#include "syscommon.h"
#include "device.h"
#include "mtk_hal_mm.h"
static PVRSRV_DEVICE_NODE* g_psSGXDevNode = NULL;

static PVRSRV_DEVICE_NODE* MTKGetDevNode(PVRSRV_DEVICE_TYPE eDeviceType)
{
    SYS_DATA    *psSysData;

    SysAcquireData(&psSysData);

    if (psSysData)
    {
        PVRSRV_DEVICE_NODE	*psDeviceNode;
        psDeviceNode = psSysData->psDeviceNodeList;
        while(psDeviceNode)
    	{
            if (psDeviceNode->sDevId.eDeviceType == eDeviceType)
    		{
                return psDeviceNode;
    		}
            psDeviceNode = psDeviceNode->psNext;
    	}
    }

	return NULL;
}

static IMG_UINT32  MTKGetGpuLoading(IMG_VOID)
{

    return 0;
}

static IMG_UINT32  MTKGetGpuMemoryStatics(IMG_VOID)
    {

#ifdef MTK_HAL_MM_STATISTIC
    return MTKGetSysRAMStats();
#else
    return 0;
#endif
    
    
}

extern unsigned int (*mtk_get_gpu_loading_fp)(void);
extern unsigned int (*mtk_get_gpu_memory_usage_fp)(void);

void MTKGpuHalInit(IMG_VOID)
{
    g_psSGXDevNode = MTKGetDevNode(PVRSRV_DEVICE_TYPE_SGX);
    mtk_get_gpu_loading_fp = MTKGetGpuLoading;
    mtk_get_gpu_memory_usage_fp = MTKGetGpuMemoryStatics;   
}
