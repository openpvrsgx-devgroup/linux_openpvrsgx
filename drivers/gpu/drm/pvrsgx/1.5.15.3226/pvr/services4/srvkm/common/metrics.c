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

#include "services_headers.h"
#include "metrics.h"

#if defined(SUPPORT_VGX)
#include "vgxapi_km.h"
#endif

#if defined(SUPPORT_SGX)
#include "sgxapi_km.h"
#endif

#if defined(DEBUG) || defined(TIMING)

static volatile IMG_UINT32 *pui32TimerRegister = 0;

#define PVRSRV_TIMER_TOTAL_IN_TICKS(X)	asTimers[X].ui32Total
#define PVRSRV_TIMER_TOTAL_IN_MS(X)		((1000*asTimers[X].ui32Total)/ui32TicksPerMS)
#define PVRSRV_TIMER_COUNT(X)			asTimers[X].ui32Count


Temporal_Data asTimers[PVRSRV_NUM_TIMERS];


IMG_UINT32 PVRSRVTimeNow(IMG_VOID)
{
	if (!pui32TimerRegister)
	{
		static IMG_BOOL bFirstTime = IMG_TRUE;

		if (bFirstTime)
		{
			PVR_DPF((PVR_DBG_ERROR,"PVRSRVTimeNow: No timer register set up"));

			bFirstTime = IMG_FALSE;
		}

		return 0;
	}

#if defined(__sh__)

	return (0xffffffff-*pui32TimerRegister);

#else

	return 0;

#endif
}


static IMG_UINT32 PVRSRVGetCPUFreq(IMG_VOID)
{
	IMG_UINT32 ui32Time1, ui32Time2;

	ui32Time1 = PVRSRVTimeNow();

	OSWaitus(1000000);

	ui32Time2 = PVRSRVTimeNow();

	PVR_DPF((PVR_DBG_WARNING, "PVRSRVGetCPUFreq: timer frequency = %d Hz", ui32Time2 - ui32Time1));

	return (ui32Time2 - ui32Time1);
}


IMG_VOID PVRSRVSetupMetricTimers(IMG_VOID *pvDevInfo)
{
	IMG_UINT32 ui32Loop;

	PVR_UNREFERENCED_PARAMETER(pvDevInfo);

	for(ui32Loop=0; ui32Loop < (PVRSRV_NUM_TIMERS); ui32Loop++)
	{
		asTimers[ui32Loop].ui32Total = 0;
		asTimers[ui32Loop].ui32Count = 0;
	}


	#if defined(__sh__)





		*TCR_2 = TIMER_DIVISOR;


		*TCOR_2 = *TCNT_2 = (IMG_UINT)0xffffffff;


		*TST_REG |= (IMG_UINT8)0x04;

		pui32TimerRegister = (IMG_UINT32 *)TCNT_2;

	#else

		pui32TimerRegister = 0;

	#endif

}


IMG_VOID PVRSRVOutputMetricTotals(IMG_VOID)
{
	IMG_UINT32 ui32TicksPerMS, ui32Loop;

	ui32TicksPerMS = PVRSRVGetCPUFreq();

	if (!ui32TicksPerMS)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVOutputMetricTotals: Failed to get CPU Freq"));
		return;
	}

	for(ui32Loop=0; ui32Loop < (PVRSRV_NUM_TIMERS); ui32Loop++)
	{
		if (asTimers[ui32Loop].ui32Count & 0x80000000L)
		{
			PVR_DPF((PVR_DBG_WARNING,"PVRSRVOutputMetricTotals: Timer %u is still ON", ui32Loop));
		}
	}
#if 0

	PVR_DPF((PVR_DBG_ERROR," Timer(%u): Total = %u",PVRSRV_TIMER_EXAMPLE_1, PVRSRV_TIMER_TOTAL_IN_TICKS(PVRSRV_TIMER_EXAMPLE_1)));
	PVR_DPF((PVR_DBG_ERROR," Timer(%u): Time = %ums",PVRSRV_TIMER_EXAMPLE_1, PVRSRV_TIMER_TOTAL_IN_MS(PVRSRV_TIMER_EXAMPLE_1)));
	PVR_DPF((PVR_DBG_ERROR," Timer(%u): Count = %u",PVRSRV_TIMER_EXAMPLE_1, PVRSRV_TIMER_COUNT(PVRSRV_TIMER_EXAMPLE_1)));
#endif
}

#endif

