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

#include "sgxdefs.h"
#include "sgxmmu.h"
#include "services_headers.h"
#include "sgxinfokm.h"
#include "sgxconfig.h"

#include "pdump_km.h"


static IMG_VOID SGXResetSoftReset(PVRSRV_SGXDEV_INFO	*psDevInfo,
								  IMG_BOOL				bResetBIF,
								  IMG_UINT32			ui32PDUMPFlags,
								  IMG_BOOL				bPDump)
{
	IMG_UINT32 ui32SoftResetRegVal;

#if defined(SGX_FEATURE_MP)
	ui32SoftResetRegVal =
					EUR_CR_MASTER_SOFT_RESET_IPF_RESET_MASK |
					EUR_CR_MASTER_SOFT_RESET_DPM_RESET_MASK  |
					EUR_CR_MASTER_SOFT_RESET_VDM_RESET_MASK;

#if defined(SGX_FEATURE_SYSTEM_CACHE)
	ui32SoftResetRegVal |= EUR_CR_MASTER_SOFT_RESET_SLC_RESET_MASK;
#endif

	if (bResetBIF)
	{
		ui32SoftResetRegVal |= EUR_CR_MASTER_SOFT_RESET_BIF_RESET_MASK;
	}

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_SOFT_RESET, ui32SoftResetRegVal);
	if (bPDump)
	{
		PDUMPREGWITHFLAGS(EUR_CR_MASTER_SOFT_RESET, ui32SoftResetRegVal, ui32PDUMPFlags);
	}
#endif

	ui32SoftResetRegVal =

					EUR_CR_SOFT_RESET_DPM_RESET_MASK |
					EUR_CR_SOFT_RESET_TA_RESET_MASK  |
					EUR_CR_SOFT_RESET_USE_RESET_MASK |
					EUR_CR_SOFT_RESET_ISP_RESET_MASK |
					EUR_CR_SOFT_RESET_TSP_RESET_MASK;

#ifdef EUR_CR_SOFT_RESET_TWOD_RESET_MASK
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_TWOD_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_TE_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_TE_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_MTE_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_MTE_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_ISP2_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_ISP2_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_PDS_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_PDS_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_PBE_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_PBE_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_CACHEL2_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_CACHEL2_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_TCU_L2_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_TCU_L2_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_UCACHEL2_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_UCACHEL2_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_MADD_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_MADD_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_ITR_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_ITR_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_TEX_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_TEX_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_IDXFIFO_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_IDXFIFO_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_VDM_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_VDM_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_DCU_L2_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_DCU_L2_RESET_MASK;
#endif
#if defined(EUR_CR_SOFT_RESET_DCU_L0L1_RESET_MASK)
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_DCU_L0L1_RESET_MASK;
#endif

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif

	if (bResetBIF)
	{
		ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_BIF_RESET_MASK;
	}

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_SOFT_RESET, ui32SoftResetRegVal);
	if (bPDump)
	{
		PDUMPREGWITHFLAGS(EUR_CR_SOFT_RESET, ui32SoftResetRegVal, ui32PDUMPFlags);
	}
}


static IMG_VOID SGXResetSleep(PVRSRV_SGXDEV_INFO	*psDevInfo,
							  IMG_UINT32			ui32PDUMPFlags,
							  IMG_BOOL				bPDump)
{
#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif


	OSWaitus(1000 * 1000000 / psDevInfo->ui32CoreClockSpeed);
	if (bPDump)
	{
		PDUMPIDLWITHFLAGS(30, ui32PDUMPFlags);
#if defined(PDUMP)
		PDumpRegRead(EUR_CR_SOFT_RESET, ui32PDUMPFlags);
#endif
	}



}


static IMG_VOID SGXResetInvalDC(PVRSRV_SGXDEV_INFO	*psDevInfo,
							    IMG_UINT32			ui32PDUMPFlags,
								IMG_BOOL			bPDump)
{
	IMG_UINT32 ui32RegVal;


#if defined(EUR_CR_BIF_CTRL_INVAL)
	ui32RegVal = EUR_CR_BIF_CTRL_INVAL_ALL_MASK;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL_INVAL, ui32RegVal);
	if (bPDump)
	{
		PDUMPREGWITHFLAGS(EUR_CR_BIF_CTRL_INVAL, ui32RegVal, ui32PDUMPFlags);
	}
#else
	ui32RegVal = EUR_CR_BIF_CTRL_INVALDC_MASK;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	if (bPDump)
	{
		PDUMPREGWITHFLAGS(EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);
	}
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, bPDump);

	ui32RegVal = 0;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	if (bPDump)
	{
		PDUMPREGWITHFLAGS(EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);
	}
#endif
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, bPDump);

#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	{



		if (PollForValueKM((IMG_UINT32 *)((IMG_UINT8*)psDevInfo->pvRegsBaseKM + EUR_CR_BIF_MEM_REQ_STAT),
							0,
							EUR_CR_BIF_MEM_REQ_STAT_READS_MASK,
							MAX_HW_TIME_US/WAIT_TRY_COUNT,
							WAIT_TRY_COUNT) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"Wait for DC invalidate failed."));
			PVR_DBG_BREAK;
		}

		if (bPDump)
		{
			PDUMPREGPOLWITHFLAGS(EUR_CR_BIF_MEM_REQ_STAT, 0, EUR_CR_BIF_MEM_REQ_STAT_READS_MASK, ui32PDUMPFlags);
		}
	}
#endif
}


IMG_VOID SGXReset(PVRSRV_SGXDEV_INFO	*psDevInfo,
				  IMG_UINT32			 ui32PDUMPFlags)
{
	IMG_UINT32 ui32RegVal;
#if defined(EUR_CR_BIF_INT_STAT_FAULT_REQ_MASK)
	const IMG_UINT32 ui32BifFaultMask = EUR_CR_BIF_INT_STAT_FAULT_REQ_MASK;
#else
	const IMG_UINT32 ui32BifFaultMask = EUR_CR_BIF_INT_STAT_FAULT_MASK;
#endif

#ifndef PDUMP
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif

	psDevInfo->ui32NumResets++;

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Start of SGX reset sequence\r\n");

#if defined(FIX_HW_BRN_23944)

	ui32RegVal = EUR_CR_BIF_CTRL_PAUSE_MASK;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_INT_STAT);
	if (ui32RegVal & ui32BifFaultMask)
	{

		ui32RegVal = EUR_CR_BIF_CTRL_PAUSE_MASK | EUR_CR_BIF_CTRL_CLEAR_FAULT_MASK;
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
		PDUMPREGWITHFLAGS(EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);

		SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

		ui32RegVal = EUR_CR_BIF_CTRL_PAUSE_MASK;
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
		PDUMPREGWITHFLAGS(EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);

		SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);
	}
#endif


	SGXResetSoftReset(psDevInfo, IMG_TRUE, ui32PDUMPFlags, IMG_TRUE);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);



#if defined(SGX_FEATURE_36BIT_MMU)

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_36BIT_ADDRESSING, EUR_CR_BIF_36BIT_ADDRESSING_ENABLE_MASK);
	PDUMPREGWITHFLAGS(EUR_CR_BIF_36BIT_ADDRESSING, EUR_CR_BIF_36BIT_ADDRESSING_ENABLE_MASK, ui32PDUMPFlags);
#endif

	ui32RegVal = 0;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);
#if defined(SGX_FEATURE_MP)
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_BIF_CTRL, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_MASTER_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);
#endif
#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_BANK_SET, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_BIF_BANK_SET, ui32RegVal, ui32PDUMPFlags);
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_BANK0, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_BIF_BANK0, ui32RegVal, ui32PDUMPFlags);
#endif

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_DIR_LIST_BASE0, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_BIF_DIR_LIST_BASE0, ui32RegVal, ui32PDUMPFlags);

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	{
		IMG_UINT32	ui32DirList, ui32DirListReg;

		for (ui32DirList = 1;
			 ui32DirList < SGX_FEATURE_BIF_NUM_DIRLISTS;
			 ui32DirList++)
		{
			ui32DirListReg = EUR_CR_BIF_DIR_LIST_BASE1 + 4 * (ui32DirList - 1);
			OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32DirListReg, ui32RegVal);
			PDUMPREGWITHFLAGS(ui32DirListReg, ui32RegVal, ui32PDUMPFlags);
		}
	}
#endif

#if defined(EUR_CR_BIF_MEM_ARB_CONFIG)


	ui32RegVal	= (12UL << EUR_CR_BIF_MEM_ARB_CONFIG_PAGE_SIZE_SHIFT) |
				  (7UL << EUR_CR_BIF_MEM_ARB_CONFIG_BEST_CNT_SHIFT) |
				  (12UL << EUR_CR_BIF_MEM_ARB_CONFIG_TTE_THRESH_SHIFT);
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_MEM_ARB_CONFIG, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_BIF_MEM_ARB_CONFIG, ui32RegVal, ui32PDUMPFlags);
#endif

#if defined(SGX_FEATURE_SYSTEM_CACHE)
#if defined(SGX_FEATURE_MP)
	#if defined(SGX_BYPASS_SYSTEM_CACHE)
		#error SGX_BYPASS_SYSTEM_CACHE not supported
	#else
		ui32RegVal = EUR_CR_MASTER_SLC_CTRL_USSE_INVAL_REQ0_MASK |
						(0xC << EUR_CR_MASTER_SLC_CTRL_ARB_PAGE_SIZE_SHIFT);
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_SLC_CTRL, ui32RegVal);
		PDUMPREG(EUR_CR_MASTER_SLC_CTRL, ui32RegVal);

		ui32RegVal = EUR_CR_MASTER_SLC_CTRL_BYPASS_BYP_CC_MASK;
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_SLC_CTRL_BYPASS, ui32RegVal);
		PDUMPREG(EUR_CR_MASTER_SLC_CTRL_BYPASS, ui32RegVal);
	#endif
#else
	#if defined(SGX_BYPASS_SYSTEM_CACHE)

		ui32RegVal = EUR_CR_MNE_CR_CTRL_BYPASS_ALL_MASK;
	#else
		#if defined(FIX_HW_BRN_26620)
			ui32RegVal = 0;
		#else

			ui32RegVal = EUR_CR_MNE_CR_CTRL_BYP_CC_MASK;
		#endif
	#endif
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MNE_CR_CTRL, ui32RegVal);
	PDUMPREG(EUR_CR_MNE_CR_CTRL, ui32RegVal);
#endif
#endif






	ui32RegVal = psDevInfo->sBIFResetPDDevPAddr.uiAddr;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_DIR_LIST_BASE0, ui32RegVal);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_FALSE);


	SGXResetSoftReset(psDevInfo, IMG_FALSE, ui32PDUMPFlags, IMG_TRUE);
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_FALSE);

	SGXResetInvalDC(psDevInfo, ui32PDUMPFlags, IMG_FALSE);



	for (;;)
	{
		IMG_UINT32 ui32BifIntStat = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_INT_STAT);
		IMG_DEV_VIRTADDR sBifFault;
		IMG_UINT32 ui32PDIndex, ui32PTIndex;

		if ((ui32BifIntStat & ui32BifFaultMask) == 0)
		{
			break;
		}




		sBifFault.uiAddr = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_FAULT);
		PVR_DPF((PVR_DBG_WARNING, "SGXReset: Page fault 0x%x/0x%x", ui32BifIntStat, sBifFault.uiAddr));
		ui32PDIndex = sBifFault.uiAddr >> (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT);
		ui32PTIndex = (sBifFault.uiAddr & SGX_MMU_PT_MASK) >> SGX_MMU_PAGE_SHIFT;


		SGXResetSoftReset(psDevInfo, IMG_TRUE, ui32PDUMPFlags, IMG_FALSE);


		psDevInfo->pui32BIFResetPD[ui32PDIndex] = (psDevInfo->sBIFResetPTDevPAddr.uiAddr
												>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
												| SGX_MMU_PDE_PAGE_SIZE_4K
												| SGX_MMU_PDE_VALID;
		psDevInfo->pui32BIFResetPT[ui32PTIndex] = (psDevInfo->sBIFResetPageDevPAddr.uiAddr
												>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
												| SGX_MMU_PTE_VALID;


		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_STATUS);
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_HOST_CLEAR, ui32RegVal);
		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_STATUS2);
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_HOST_CLEAR2, ui32RegVal);

		SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_FALSE);


		SGXResetSoftReset(psDevInfo, IMG_FALSE, ui32PDUMPFlags, IMG_FALSE);
		SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_FALSE);


		SGXResetInvalDC(psDevInfo, ui32PDUMPFlags, IMG_FALSE);


		psDevInfo->pui32BIFResetPD[ui32PDIndex] = 0;
		psDevInfo->pui32BIFResetPT[ui32PTIndex] = 0;
	}




	#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)

	ui32RegVal = (SGX_BIF_DIR_LIST_INDEX_EDM << EUR_CR_BIF_BANK0_INDEX_EDM_SHIFT);

	#if defined(SGX_FEATURE_2D_HARDWARE)

	ui32RegVal |= (SGX_BIF_DIR_LIST_INDEX_EDM << EUR_CR_BIF_BANK0_INDEX_2D_SHIFT);
	#endif

	#if defined(FIX_HW_BRN_23410)

	ui32RegVal |= (SGX_BIF_DIR_LIST_INDEX_EDM << EUR_CR_BIF_BANK0_INDEX_TA_SHIFT);
	#endif

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_BANK0, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_BIF_BANK0, ui32RegVal, ui32PDUMPFlags);
	#endif

	{
		IMG_UINT32	ui32EDMDirListReg;


		#if (SGX_BIF_DIR_LIST_INDEX_EDM == 0)
		ui32EDMDirListReg = EUR_CR_BIF_DIR_LIST_BASE0;
		#else

		ui32EDMDirListReg = EUR_CR_BIF_DIR_LIST_BASE1 + 4 * (SGX_BIF_DIR_LIST_INDEX_EDM - 1);
		#endif

#if defined(FIX_HW_BRN_28011)
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_DIR_LIST_BASE0, psDevInfo->sKernelPDDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT);
		PDUMPPDREGWITHFLAGS(EUR_CR_BIF_DIR_LIST_BASE0, psDevInfo->sKernelPDDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT, ui32PDUMPFlags, PDUMP_PD_UNIQUETAG);
#endif

		OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32EDMDirListReg, psDevInfo->sKernelPDDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT);
		PDUMPPDREGWITHFLAGS(ui32EDMDirListReg, psDevInfo->sKernelPDDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT, ui32PDUMPFlags, PDUMP_PD_UNIQUETAG);
	}

#ifdef SGX_FEATURE_2D_HARDWARE

	#if ((SGX_2D_HEAP_BASE & ~EUR_CR_BIF_TWOD_REQ_BASE_ADDR_MASK) != 0)
		#error "SGXReset: SGX_2D_HEAP_BASE doesn't match EUR_CR_BIF_TWOD_REQ_BASE_ADDR_MASK alignment"
	#endif

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_TWOD_REQ_BASE, SGX_2D_HEAP_BASE);
	PDUMPREGWITHFLAGS(EUR_CR_BIF_TWOD_REQ_BASE, SGX_2D_HEAP_BASE, ui32PDUMPFlags);
#endif


	SGXResetInvalDC(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	PVR_DPF((PVR_DBG_MESSAGE,"Soft Reset of SGX"));
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);


	ui32RegVal = 0;
#if defined(SGX_FEATURE_MP)
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_SOFT_RESET, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_MASTER_SOFT_RESET, ui32RegVal, ui32PDUMPFlags);
#endif
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_SOFT_RESET, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_SOFT_RESET, ui32RegVal, ui32PDUMPFlags);


	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "End of SGX reset sequence\r\n");
}


