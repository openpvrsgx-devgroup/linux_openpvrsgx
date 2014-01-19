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

#if defined(DEBUG) || defined (INTERNAL_TEST)
#define DEBUG_SET_OFFSET	OPTIONS_BIT0
#define OPTIONS_BIT0		0x1
#else
#define OPTIONS_BIT0		0x0
#endif

#if defined(PDUMP) || defined (INTERNAL_TEST)
#define PDUMP_SET_OFFSET	OPTIONS_BIT1
#define OPTIONS_BIT1		(0x1 << 1)
#else
#define OPTIONS_BIT1		0x0
#endif

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG) || defined (INTERNAL_TEST)
#define PVRSRV_USSE_EDM_STATUS_DEBUG_SET_OFFSET		OPTIONS_BIT2
#define OPTIONS_BIT2		(0x1 << 2)
#else
#define OPTIONS_BIT2		0x0
#endif

#if defined(SUPPORT_HW_RECOVERY) || defined (INTERNAL_TEST)
#define SUPPORT_HW_RECOVERY_SET_OFFSET	OPTIONS_BIT3
#define OPTIONS_BIT3		(0x1 << 3)
#else
#define OPTIONS_BIT3		0x0
#endif



#if defined(PVR_SECURE_HANDLES) || defined (INTERNAL_TEST)
#define PVR_SECURE_HANDLES_SET_OFFSET	OPTIONS_BIT4
#define OPTIONS_BIT4		(0x1 << 4)
#else
#define OPTIONS_BIT4		0x0
#endif

#if defined(SGX_BYPASS_SYSTEM_CACHE) || defined (INTERNAL_TEST)
#define SGX_BYPASS_SYSTEM_CACHE_SET_OFFSET	OPTIONS_BIT5
#define OPTIONS_BIT5		(0x1 << 5)
#else
#define OPTIONS_BIT5		0x0
#endif

#if defined(SGX_DMS_AGE_ENABLE) || defined (INTERNAL_TEST)
#define SGX_DMS_AGE_ENABLE_SET_OFFSET	OPTIONS_BIT6
#define OPTIONS_BIT6		(0x1 << 6)
#else
#define OPTIONS_BIT6		0x0
#endif

#if defined(SGX_FAST_DPM_INIT) || defined (INTERNAL_TEST)
#define SGX_FAST_DPM_INIT_SET_OFFSET	OPTIONS_BIT8
#define OPTIONS_BIT8		(0x1 << 8)
#else
#define OPTIONS_BIT8		0x0
#endif

#if defined(SGX_FEATURE_DCU) || defined (INTERNAL_TEST)
#define SGX_FEATURE_DCU_SET_OFFSET	OPTIONS_BIT9
#define OPTIONS_BIT9		(0x1 << 9)
#else
#define OPTIONS_BIT9		0x0
#endif

#if defined(SGX_FEATURE_MP) || defined (INTERNAL_TEST)
#define SGX_FEATURE_MP_SET_OFFSET	OPTIONS_BIT10
#define OPTIONS_BIT10		(0x1 << 10)
#else
#define OPTIONS_BIT10		0x0
#endif

#if defined(SGX_FEATURE_MULTITHREADED_UKERNEL) || defined (INTERNAL_TEST)
#define SGX_FEATURE_MULTITHREADED_UKERNEL_SET_OFFSET	OPTIONS_BIT11
#define OPTIONS_BIT11		(0x1 << 11)
#else
#define OPTIONS_BIT11		0x0
#endif



#if defined(SGX_FEATURE_OVERLAPPED_SPM) || defined (INTERNAL_TEST)
#define SGX_FEATURE_OVERLAPPED_SPM_SET_OFFSET	OPTIONS_BIT12
#define OPTIONS_BIT12		(0x1 << 12)
#else
#define OPTIONS_BIT12		0x0
#endif


#if defined(SGX_FEATURE_SYSTEM_CACHE) || defined (INTERNAL_TEST)
#define SGX_FEATURE_SYSTEM_CACHE_SET_OFFSET	OPTIONS_BIT13
#define OPTIONS_BIT13		(0x1 << 13)
#else
#define OPTIONS_BIT13		0x0
#endif

#if defined(SGX_SUPPORT_HWPROFILING) || defined (INTERNAL_TEST)
#define SGX_SUPPORT_HWPROFILING_SET_OFFSET	OPTIONS_BIT14
#define OPTIONS_BIT14		(0x1 << 14)
#else
#define OPTIONS_BIT14		0x0
#endif



#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT) || defined (INTERNAL_TEST)
#define SUPPORT_ACTIVE_POWER_MANAGEMENT_SET_OFFSET	OPTIONS_BIT15
#define OPTIONS_BIT15		(0x1 << 15)
#else
#define OPTIONS_BIT15		0x0
#endif

#if defined(SUPPORT_DISPLAYCONTROLLER_TILING) || defined (INTERNAL_TEST)
#define SUPPORT_DISPLAYCONTROLLER_TILING_SET_OFFSET	OPTIONS_BIT16
#define OPTIONS_BIT16		(0x1 << 16)
#else
#define OPTIONS_BIT16		0x0
#endif

#if defined(SUPPORT_PERCONTEXT_PB) || defined (INTERNAL_TEST)
#define SUPPORT_PERCONTEXT_PB_SET_OFFSET	OPTIONS_BIT17
#define OPTIONS_BIT17		(0x1 << 17)
#else
#define OPTIONS_BIT17		0x0
#endif

#if defined(SUPPORT_SGX_HWPERF) || defined (INTERNAL_TEST)
#define SUPPORT_SGX_HWPERF_SET_OFFSET	OPTIONS_BIT18
#define OPTIONS_BIT18		(0x1 << 18)
#else
#define OPTIONS_BIT18		0x0
#endif



#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE) || defined (INTERNAL_TEST)
#define SUPPORT_SGX_MMU_DUMMY_PAGE_SET_OFFSET	OPTIONS_BIT19
#define OPTIONS_BIT19		(0x1 << 19)
#else
#define OPTIONS_BIT19		0x0
#endif

#if defined(SUPPORT_SGX_PRIORITY_SCHEDULING) || defined (INTERNAL_TEST)
#define SUPPORT_SGX_PRIORITY_SCHEDULING_SET_OFFSET	OPTIONS_BIT20
#define OPTIONS_BIT20		(0x1 << 20)
#else
#define OPTIONS_BIT20		0x0
#endif

#if defined(SGX_LOW_LATENCY_SCHEDULING) || defined (INTERNAL_TEST)
#define SUPPORT_SGX_LOW_LATENCY_SCHEDULING_SET_OFFSET	OPTIONS_BIT21
#define OPTIONS_BIT21		(0x1 << 21)
#else
#define OPTIONS_BIT21		0x0
#endif

#if defined(USE_SUPPORT_NO_TA3D_OVERLAP) || defined (INTERNAL_TEST)
#define USE_SUPPORT_NO_TA3D_OVERLAP_SET_OFFSET	OPTIONS_BIT22
#define OPTIONS_BIT22		(0x1 << 22)
#else
#define OPTIONS_BIT22		0x0
#endif


#if defined(SGX_FEATURE_MP) || defined (INTERNAL_TEST)
#define OPTIONS_HIGHBYTE ((SGX_FEATURE_MP_CORE_COUNT-1) << SGX_FEATURE_MP_CORE_COUNT_SET_OFFSET)
#define SGX_FEATURE_MP_CORE_COUNT_SET_OFFSET	28UL
#define SGX_FEATURE_MP_CORE_COUNT_SET_MASK		0xFF
#else
#define OPTIONS_HIGHBYTE	0x0
#endif



#define SGX_BUILD_OPTIONS	\
	OPTIONS_BIT0 |\
	OPTIONS_BIT1 |\
	OPTIONS_BIT2 |\
	OPTIONS_BIT3 |\
	OPTIONS_BIT4 |\
	OPTIONS_BIT5 |\
	OPTIONS_BIT6 |\
	OPTIONS_BIT8 |\
	OPTIONS_BIT9 |\
	OPTIONS_BIT10 |\
	OPTIONS_BIT11 |\
	OPTIONS_BIT12 |\
	OPTIONS_BIT13 |\
	OPTIONS_BIT14 |\
	OPTIONS_BIT15 |\
	OPTIONS_BIT16 |\
	OPTIONS_BIT17 |\
	OPTIONS_BIT18 |\
	OPTIONS_BIT19 |\
	OPTIONS_BIT20 |\
	OPTIONS_BIT21 |\
	OPTIONS_HIGHBYTE

