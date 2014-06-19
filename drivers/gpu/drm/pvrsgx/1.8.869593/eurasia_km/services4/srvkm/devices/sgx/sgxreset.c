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

#include "sgxdefs.h"
#include "sgxmmu.h"
#include "services_headers.h"
#include "sgxinfokm.h"
#include "sgxconfig.h"
#include "sgxutils.h"

#include "pdump_km.h"


IMG_VOID SGXInitClocks(PVRSRV_SGXDEV_INFO	*psDevInfo,
					   IMG_UINT32			ui32PDUMPFlags)
{
	IMG_UINT32	ui32RegVal;
	
#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif 
	
	ui32RegVal = psDevInfo->ui32ClkGateCtl;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_CLKGATECTL, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_CLKGATECTL, ui32RegVal, ui32PDUMPFlags);

#if defined(EUR_CR_CLKGATECTL2)
	ui32RegVal = psDevInfo->ui32ClkGateCtl2;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_CLKGATECTL2, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_CLKGATECTL2, ui32RegVal, ui32PDUMPFlags);
#endif
}


static IMG_VOID SGXResetInitBIFContexts(PVRSRV_SGXDEV_INFO	*psDevInfo,
										IMG_UINT32			ui32PDUMPFlags)
{
	IMG_UINT32	ui32RegVal;

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif 

	ui32RegVal = 0;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Initialise the BIF bank settings\r\n");
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_BANK_SET, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_BANK_SET, ui32RegVal, ui32PDUMPFlags);
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_BANK0, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_BANK0, ui32RegVal, ui32PDUMPFlags);
#endif 

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Initialise the BIF directory list\r\n");
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_DIR_LIST_BASE0, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_DIR_LIST_BASE0, ui32RegVal, ui32PDUMPFlags);

#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	{
		IMG_UINT32	ui32DirList, ui32DirListReg;

		for (ui32DirList = 1;
			 ui32DirList < SGX_FEATURE_BIF_NUM_DIRLISTS;
			 ui32DirList++)
		{
			ui32DirListReg = EUR_CR_BIF_DIR_LIST_BASE1 + 4 * (ui32DirList - 1);
			OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32DirListReg, ui32RegVal);
			PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, ui32DirListReg, ui32RegVal, ui32PDUMPFlags);
		}
	}
#endif 
}


static IMG_VOID SGXResetSetupBIFContexts(PVRSRV_SGXDEV_INFO	*psDevInfo,
										 IMG_UINT32			ui32PDUMPFlags)
{
	IMG_UINT32	ui32RegVal;

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif 
	
	#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	
	ui32RegVal = (SGX_BIF_DIR_LIST_INDEX_EDM << EUR_CR_BIF_BANK0_INDEX_EDM_SHIFT);

	#if defined(SGX_FEATURE_2D_HARDWARE) && !defined(SGX_FEATURE_PTLA)
	
	ui32RegVal |= (SGX_BIF_DIR_LIST_INDEX_EDM << EUR_CR_BIF_BANK0_INDEX_2D_SHIFT);
	#endif 

	#if defined(FIX_HW_BRN_23410)
	
	ui32RegVal |= (SGX_BIF_DIR_LIST_INDEX_EDM << EUR_CR_BIF_BANK0_INDEX_TA_SHIFT);
	#endif 

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_BANK0, ui32RegVal);
	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Set up EDM requestor page table in BIF\r\n");
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_BANK0, ui32RegVal, ui32PDUMPFlags);
	#endif 

	{
		IMG_UINT32	ui32EDMDirListReg;

		
		#if (SGX_BIF_DIR_LIST_INDEX_EDM == 0)
		ui32EDMDirListReg = EUR_CR_BIF_DIR_LIST_BASE0;
		#else
		
		ui32EDMDirListReg = EUR_CR_BIF_DIR_LIST_BASE1 + 4 * (SGX_BIF_DIR_LIST_INDEX_EDM - 1);
		#endif 

		ui32RegVal = psDevInfo->sKernelPDDevPAddr.uiAddr >> SGX_MMU_PDE_ADDR_ALIGNSHIFT;
		
#if defined(FIX_HW_BRN_28011)
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_DIR_LIST_BASE0, ui32RegVal);
		PDUMPPDREGWITHFLAGS(&psDevInfo->sMMUAttrib, EUR_CR_BIF_DIR_LIST_BASE0, ui32RegVal, ui32PDUMPFlags, PDUMP_PD_UNIQUETAG);
#endif

		OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32EDMDirListReg, ui32RegVal);
		PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Initialise the EDM's directory list base\r\n");
		PDUMPPDREGWITHFLAGS(&psDevInfo->sMMUAttrib, ui32EDMDirListReg, ui32RegVal, ui32PDUMPFlags, PDUMP_PD_UNIQUETAG);
	}
}


static IMG_VOID SGXResetSleep(PVRSRV_SGXDEV_INFO	*psDevInfo,
							  IMG_UINT32			ui32PDUMPFlags,
							  IMG_BOOL				bPDump)
{
#if defined(PDUMP) || defined(EMULATOR)
	IMG_UINT32	ui32ReadRegister;

	#if defined(SGX_FEATURE_MP)
	ui32ReadRegister = EUR_CR_MASTER_SOFT_RESET;
	#else
	ui32ReadRegister = EUR_CR_SOFT_RESET;
	#endif 
#endif

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif 

	
	SGXWaitClocks(psDevInfo, 100);
	if (bPDump)
	{
		PDUMPIDLWITHFLAGS(30, ui32PDUMPFlags);
#if defined(PDUMP)
		PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Read back to flush the register writes\r\n");
		PDumpRegRead(SGX_PDUMPREG_NAME, ui32ReadRegister, ui32PDUMPFlags);
#endif
	}

#if defined(EMULATOR)
	

	OSReadHWReg(psDevInfo->pvRegsBaseKM, ui32ReadRegister);
#endif
}


#if !defined(SGX_FEATURE_MP)
static IMG_VOID SGXResetSoftReset(PVRSRV_SGXDEV_INFO	*psDevInfo,
								  IMG_BOOL				bResetBIF,
								  IMG_UINT32			ui32PDUMPFlags,
								  IMG_BOOL				bPDump)
{
	IMG_UINT32 ui32SoftResetRegVal;

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
		PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_SOFT_RESET, ui32SoftResetRegVal, ui32PDUMPFlags);
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
		PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL_INVAL, ui32RegVal, ui32PDUMPFlags);
	}
#else
	ui32RegVal = EUR_CR_BIF_CTRL_INVALDC_MASK;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	if (bPDump)
	{
		PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);
	}

	ui32RegVal = 0;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	if (bPDump)
	{
		PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);
	}
#endif
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, bPDump);

#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	{
		


		if (PollForValueKM((IMG_UINT32 *)((IMG_UINT8*)psDevInfo->pvRegsBaseKM + EUR_CR_BIF_MEM_REQ_STAT),
							0,
							EUR_CR_BIF_MEM_REQ_STAT_READS_MASK,
							MAX_HW_TIME_US,
							MAX_HW_TIME_US/WAIT_TRY_COUNT,
							IMG_FALSE) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"Wait for DC invalidate failed."));
			PVR_DBG_BREAK;
		}

		if (bPDump)
		{
			PDUMPREGPOLWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_MEM_REQ_STAT, 0, EUR_CR_BIF_MEM_REQ_STAT_READS_MASK, ui32PDUMPFlags, PDUMP_POLL_OPERATOR_EQUAL);
		}
	}
#endif 
}
#endif 


IMG_VOID SGXReset(PVRSRV_SGXDEV_INFO	*psDevInfo,
				  IMG_BOOL				bHardwareRecovery,
				  IMG_UINT32			ui32PDUMPFlags)
#if !defined(SGX_FEATURE_MP)
{
	IMG_UINT32 ui32RegVal;
#if defined(EUR_CR_BIF_INT_STAT_FAULT_REQ_MASK)
	const IMG_UINT32 ui32BifFaultMask = EUR_CR_BIF_INT_STAT_FAULT_REQ_MASK;
#else
	const IMG_UINT32 ui32BifFaultMask = EUR_CR_BIF_INT_STAT_FAULT_MASK;
#endif

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif 

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Start of SGX reset sequence\r\n");

#if defined(FIX_HW_BRN_23944)
	
	ui32RegVal = EUR_CR_BIF_CTRL_PAUSE_MASK;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_INT_STAT);
	if (ui32RegVal & ui32BifFaultMask)
	{
		
		ui32RegVal = EUR_CR_BIF_CTRL_PAUSE_MASK | EUR_CR_BIF_CTRL_CLEAR_FAULT_MASK;
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
		PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);

		SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

		ui32RegVal = EUR_CR_BIF_CTRL_PAUSE_MASK;
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
		PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);

		SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);
	}
#endif 

	
	SGXResetSoftReset(psDevInfo, IMG_TRUE, ui32PDUMPFlags, IMG_TRUE);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	

#if defined(SGX_FEATURE_36BIT_MMU)
	
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_36BIT_ADDRESSING, EUR_CR_BIF_36BIT_ADDRESSING_ENABLE_MASK);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_36BIT_ADDRESSING, EUR_CR_BIF_36BIT_ADDRESSING_ENABLE_MASK, ui32PDUMPFlags);
#endif

	SGXResetInitBIFContexts(psDevInfo, ui32PDUMPFlags);

#if defined(EUR_CR_BIF_MEM_ARB_CONFIG)
	

	ui32RegVal	= (12UL << EUR_CR_BIF_MEM_ARB_CONFIG_PAGE_SIZE_SHIFT) |
				  (7UL << EUR_CR_BIF_MEM_ARB_CONFIG_BEST_CNT_SHIFT) |
				  (12UL << EUR_CR_BIF_MEM_ARB_CONFIG_TTE_THRESH_SHIFT);
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_MEM_ARB_CONFIG, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_MEM_ARB_CONFIG, ui32RegVal, ui32PDUMPFlags);
#endif 

#if defined(SGX_FEATURE_SYSTEM_CACHE)
	#if defined(SGX_BYPASS_SYSTEM_CACHE)
		
		ui32RegVal = MNE_CR_CTRL_BYPASS_ALL_MASK;
	#else
		#if defined(FIX_HW_BRN_26620)
			ui32RegVal = 0;
		#else
			
			ui32RegVal = MNE_CR_CTRL_BYP_CC_MASK;
		#endif
		#if defined(FIX_HW_BRN_34028)
			
			ui32RegVal |= (8 << MNE_CR_CTRL_BYPASS_SHIFT);
		#endif
	#endif 
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, MNE_CR_CTRL, ui32RegVal);
	PDUMPREG(SGX_PDUMPREG_NAME, MNE_CR_CTRL, ui32RegVal);
#endif 

	if (bHardwareRecovery)
	{
		






		ui32RegVal = (IMG_UINT32)psDevInfo->sBIFResetPDDevPAddr.uiAddr;
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
	}
	else
	{
		
		SGXResetSoftReset(psDevInfo, IMG_FALSE, ui32PDUMPFlags, IMG_TRUE);
		SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_FALSE);
	}	

	

	SGXResetSetupBIFContexts(psDevInfo, ui32PDUMPFlags);

#if defined(SGX_FEATURE_2D_HARDWARE) && !defined(SGX_FEATURE_PTLA)
	
	#if ((SGX_2D_HEAP_BASE & ~EUR_CR_BIF_TWOD_REQ_BASE_ADDR_MASK) != 0)
		#error "SGXReset: SGX_2D_HEAP_BASE doesn't match EUR_CR_BIF_TWOD_REQ_BASE_ADDR_MASK alignment"
	#endif
	
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_TWOD_REQ_BASE, SGX_2D_HEAP_BASE);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_BIF_TWOD_REQ_BASE, SGX_2D_HEAP_BASE, ui32PDUMPFlags);
#endif

	
	SGXResetInvalDC(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	PVR_DPF((PVR_DBG_MESSAGE,"Soft Reset of SGX"));

	
	ui32RegVal = 0;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_SOFT_RESET, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_SOFT_RESET, ui32RegVal, ui32PDUMPFlags);

	
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "End of SGX reset sequence\r\n");
}

#else

{
	IMG_UINT32 ui32RegVal;
	
	PVR_UNREFERENCED_PARAMETER(bHardwareRecovery);

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif 

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Start of SGX MP reset sequence\r\n");

	
	ui32RegVal = EUR_CR_MASTER_SOFT_RESET_BIF_RESET_MASK  |
				 EUR_CR_MASTER_SOFT_RESET_IPF_RESET_MASK  |
				 EUR_CR_MASTER_SOFT_RESET_DPM_RESET_MASK  |
				 EUR_CR_MASTER_SOFT_RESET_MCI_RESET_MASK  |
				 EUR_CR_MASTER_SOFT_RESET_VDM_RESET_MASK;

#if defined(SGX_FEATURE_PTLA)
	ui32RegVal |= EUR_CR_MASTER_SOFT_RESET_PTLA_RESET_MASK;
#endif
#if defined(SGX_FEATURE_SYSTEM_CACHE)
	ui32RegVal |= EUR_CR_MASTER_SOFT_RESET_SLC_RESET_MASK;
#endif

	
	ui32RegVal |= EUR_CR_MASTER_SOFT_RESET_CORE_RESET_MASK(0)  |
				  EUR_CR_MASTER_SOFT_RESET_CORE_RESET_MASK(1)  |
				  EUR_CR_MASTER_SOFT_RESET_CORE_RESET_MASK(2)  |
				  EUR_CR_MASTER_SOFT_RESET_CORE_RESET_MASK(3);

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_SOFT_RESET, ui32RegVal);
	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Soft reset hydra partition, hard reset the cores\r\n");
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_MASTER_SOFT_RESET, ui32RegVal, ui32PDUMPFlags);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	ui32RegVal = 0;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_BIF_CTRL, ui32RegVal);
	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Initialise the hydra BIF control\r\n");
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_MASTER_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);

#if defined(SGX_FEATURE_SYSTEM_CACHE)
	#if defined(SGX_BYPASS_SYSTEM_CACHE)
		ui32RegVal = EUR_CR_MASTER_SLC_CTRL_BYPASS_ALL_MASK;
	#else
		ui32RegVal = EUR_CR_MASTER_SLC_CTRL_USSE_INVAL_REQ0_MASK |
		#if defined(FIX_HW_BRN_30954)
						EUR_CR_MASTER_SLC_CTRL_DISABLE_REORDERING_MASK |
		#endif
		#if defined(PVR_SLC_8KB_ADDRESS_MODE)
						(4 << EUR_CR_MASTER_SLC_CTRL_ADDR_DECODE_MODE_SHIFT) |
		#endif
		#if defined(FIX_HW_BRN_33809)
			(2 << EUR_CR_MASTER_SLC_CTRL_ADDR_DECODE_MODE_SHIFT) |
		#endif
						(0xC << EUR_CR_MASTER_SLC_CTRL_ARB_PAGE_SIZE_SHIFT);
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_SLC_CTRL, ui32RegVal);
		PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Initialise the hydra SLC control\r\n");
		PDUMPREG(SGX_PDUMPREG_NAME, EUR_CR_MASTER_SLC_CTRL, ui32RegVal);

		ui32RegVal = EUR_CR_MASTER_SLC_CTRL_BYPASS_BYP_CC_MASK;
	#if defined(FIX_HW_BRN_31620)
		ui32RegVal |= EUR_CR_MASTER_SLC_CTRL_BYPASS_REQ_MMU_MASK;
	#endif
	#if defined(FIX_HW_BRN_31195)
		ui32RegVal |= EUR_CR_MASTER_SLC_CTRL_BYPASS_REQ_USE0_MASK |
				EUR_CR_MASTER_SLC_CTRL_BYPASS_REQ_USE1_MASK |
				EUR_CR_MASTER_SLC_CTRL_BYPASS_REQ_USE2_MASK |
				EUR_CR_MASTER_SLC_CTRL_BYPASS_REQ_USE3_MASK |
				EUR_CR_MASTER_SLC_CTRL_BYPASS_REQ_TA_MASK;
	#endif
	#endif 
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_SLC_CTRL_BYPASS, ui32RegVal);
		PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Initialise the hydra SLC bypass control\r\n");
		PDUMPREG(SGX_PDUMPREG_NAME, EUR_CR_MASTER_SLC_CTRL_BYPASS, ui32RegVal);
#endif 

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	
	ui32RegVal = 0;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_SOFT_RESET, ui32RegVal);
	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Remove the resets from all of SGX\r\n");
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_MASTER_SOFT_RESET, ui32RegVal, ui32PDUMPFlags);
	
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Turn on the slave cores' clock gating\r\n");
	SGXInitClocks(psDevInfo, ui32PDUMPFlags);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "Initialise the slave BIFs\r\n");

#if defined(FIX_HW_BRN_31278) || defined(FIX_HW_BRN_31620) || defined(FIX_HW_BRN_31671) || defined(FIX_HW_BRN_32085)
	#if defined(FIX_HW_BRN_31278) || defined(FIX_HW_BRN_32085)
	
	ui32RegVal = (1<<EUR_CR_MASTER_BIF_MMU_CTRL_ADDR_HASH_MODE_SHIFT);
	#else
	ui32RegVal = (1<<EUR_CR_MASTER_BIF_MMU_CTRL_ADDR_HASH_MODE_SHIFT) | EUR_CR_MASTER_BIF_MMU_CTRL_PREFETCHING_ON_MASK; 
	#endif
	#if !defined(FIX_HW_BRN_31620) && !defined(FIX_HW_BRN_31671)
	
	ui32RegVal |= EUR_CR_MASTER_BIF_MMU_CTRL_ENABLE_DC_TLB_MASK;
	#endif

	
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_MASTER_BIF_MMU_CTRL, ui32RegVal);
	PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, EUR_CR_MASTER_BIF_MMU_CTRL, ui32RegVal, ui32PDUMPFlags);

	#if defined(FIX_HW_BRN_31278) || defined(FIX_HW_BRN_32085)
	
	ui32RegVal = (1<<EUR_CR_BIF_MMU_CTRL_ADDR_HASH_MODE_SHIFT);
	#else
	ui32RegVal = (1<<EUR_CR_BIF_MMU_CTRL_ADDR_HASH_MODE_SHIFT) | EUR_CR_BIF_MMU_CTRL_PREFETCHING_ON_MASK; 
	#endif
	#if !defined(FIX_HW_BRN_31620) && !defined(FIX_HW_BRN_31671)
	
	ui32RegVal |= EUR_CR_BIF_MMU_CTRL_ENABLE_DC_TLB_MASK;
	#endif

	
	{
		IMG_UINT32 ui32Core;

		for (ui32Core=0;ui32Core<SGX_FEATURE_MP_CORE_COUNT;ui32Core++)
		{
			OSWriteHWReg(psDevInfo->pvRegsBaseKM, SGX_MP_CORE_SELECT(EUR_CR_BIF_MMU_CTRL, ui32Core), ui32RegVal);
			PDUMPREGWITHFLAGS(SGX_PDUMPREG_NAME, SGX_MP_CORE_SELECT(EUR_CR_BIF_MMU_CTRL, ui32Core), ui32RegVal, ui32PDUMPFlags);
		}
	}
#endif

	SGXResetInitBIFContexts(psDevInfo, ui32PDUMPFlags);
	SGXResetSetupBIFContexts(psDevInfo, ui32PDUMPFlags);
	
	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "End of SGX MP reset sequence\r\n");
}	
#endif 


