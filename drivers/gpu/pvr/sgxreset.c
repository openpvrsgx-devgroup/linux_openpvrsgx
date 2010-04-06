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

#include "sgxdefs.h"
#include "sgxmmu.h"
#include "services_headers.h"
#include "sgxinfokm.h"
#include "sgxconfig.h"

#include "pdump_km.h"

static void SGXResetSoftReset(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				  IMG_BOOL bResetBIF, u32 ui32PDUMPFlags,
				  IMG_BOOL bPDump)
{
	u32 ui32SoftResetRegVal =
	    EUR_CR_SOFT_RESET_DPM_RESET_MASK |
	    EUR_CR_SOFT_RESET_TA_RESET_MASK |
	    EUR_CR_SOFT_RESET_USE_RESET_MASK |
	    EUR_CR_SOFT_RESET_ISP_RESET_MASK | EUR_CR_SOFT_RESET_TSP_RESET_MASK;

#ifdef EUR_CR_SOFT_RESET_TWOD_RESET_MASK
	ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_TWOD_RESET_MASK;
#endif

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif

	if (bResetBIF)
		ui32SoftResetRegVal |= EUR_CR_SOFT_RESET_BIF_RESET_MASK;

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_SOFT_RESET,
		     ui32SoftResetRegVal);
	if (bPDump)
		PDUMPREGWITHFLAGS(EUR_CR_SOFT_RESET, ui32SoftResetRegVal,
				  ui32PDUMPFlags);
}

static void SGXResetSleep(struct PVRSRV_SGXDEV_INFO *psDevInfo,
			      u32 ui32PDUMPFlags, IMG_BOOL bPDump)
{
#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif

	OSWaitus(1000 * 1000000 / psDevInfo->ui32CoreClockSpeed);
	if (bPDump) {
		PDUMPIDLWITHFLAGS(30, ui32PDUMPFlags);
#if defined(PDUMP)
		PDumpRegRead(EUR_CR_SOFT_RESET, ui32PDUMPFlags);
#endif
	}

}

static void SGXResetInvalDC(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				u32 ui32PDUMPFlags, IMG_BOOL bPDump)
{
	u32 ui32RegVal;

	ui32RegVal = EUR_CR_BIF_CTRL_INVALDC_MASK;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	if (bPDump)
		PDUMPREGWITHFLAGS(EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, bPDump);

	ui32RegVal = 0;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32RegVal);
	if (bPDump)
		PDUMPREGWITHFLAGS(EUR_CR_BIF_CTRL, ui32RegVal, ui32PDUMPFlags);
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, bPDump);

	if (PollForValueKM(
	    (u32 __iomem *)((u8 __iomem *)psDevInfo->pvRegsBaseKM +
						EUR_CR_BIF_MEM_REQ_STAT),
	     0, EUR_CR_BIF_MEM_REQ_STAT_READS_MASK,
	     MAX_HW_TIME_US / WAIT_TRY_COUNT, WAIT_TRY_COUNT) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "Wait for DC invalidate failed.");
		PVR_DBG_BREAK;
	}

	if (bPDump)
		PDUMPREGPOLWITHFLAGS(EUR_CR_BIF_MEM_REQ_STAT, 0,
				     EUR_CR_BIF_MEM_REQ_STAT_READS_MASK,
				     ui32PDUMPFlags);
}

void SGXReset(struct PVRSRV_SGXDEV_INFO *psDevInfo, u32 ui32PDUMPFlags)
{
	u32 ui32RegVal;

	const u32 ui32BifFaultMask = EUR_CR_BIF_INT_STAT_FAULT_MASK;


#ifndef PDUMP
	PVR_UNREFERENCED_PARAMETER(ui32PDUMPFlags);
#endif

	psDevInfo->ui32NumResets++;

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags,
			      "Start of SGX reset sequence\r\n");

	SGXResetSoftReset(psDevInfo, IMG_TRUE, ui32PDUMPFlags, IMG_TRUE);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);


	ui32RegVal = psDevInfo->sBIFResetPDDevPAddr.uiAddr;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_DIR_LIST_BASE0,
		     ui32RegVal);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_FALSE);

	SGXResetSoftReset(psDevInfo, IMG_FALSE, ui32PDUMPFlags, IMG_TRUE);
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_FALSE);

	SGXResetInvalDC(psDevInfo, ui32PDUMPFlags, IMG_FALSE);

	for (;;) {
		u32 ui32BifIntStat =
		    OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_INT_STAT);
		struct IMG_DEV_VIRTADDR sBifFault;
		u32 ui32PDIndex, ui32PTIndex;

		if ((ui32BifIntStat & ui32BifFaultMask) == 0)
			break;

		sBifFault.uiAddr =
		    OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_FAULT);
		PVR_DPF(PVR_DBG_WARNING, "SGXReset: Page fault 0x%x/0x%x",
			 ui32BifIntStat, sBifFault.uiAddr);
		ui32PDIndex =
		    sBifFault.uiAddr >> (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT);
		ui32PTIndex =
		    (sBifFault.uiAddr & SGX_MMU_PT_MASK) >> SGX_MMU_PAGE_SHIFT;

		SGXResetSoftReset(psDevInfo, IMG_TRUE, ui32PDUMPFlags,
				  IMG_FALSE);

		psDevInfo->pui32BIFResetPD[ui32PDIndex] =
		    psDevInfo->sBIFResetPTDevPAddr.uiAddr |
				SGX_MMU_PDE_PAGE_SIZE_4K | SGX_MMU_PDE_VALID;
		psDevInfo->pui32BIFResetPT[ui32PTIndex] =
		    psDevInfo->sBIFResetPageDevPAddr.uiAddr | SGX_MMU_PTE_VALID;

		ui32RegVal =
		    OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_STATUS);
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_HOST_CLEAR,
			     ui32RegVal);
		ui32RegVal =
		    OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_STATUS2);
		OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_HOST_CLEAR2,
			     ui32RegVal);

		SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_FALSE);

		SGXResetSoftReset(psDevInfo, IMG_FALSE, ui32PDUMPFlags,
				  IMG_FALSE);
		SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_FALSE);

		SGXResetInvalDC(psDevInfo, ui32PDUMPFlags, IMG_FALSE);

		psDevInfo->pui32BIFResetPD[ui32PDIndex] = 0;
		psDevInfo->pui32BIFResetPT[ui32PTIndex] = 0;
	}

	{
		u32 ui32EDMDirListReg;

#if (SGX_BIF_DIR_LIST_INDEX_EDM == 0)
		ui32EDMDirListReg = EUR_CR_BIF_DIR_LIST_BASE0;
#else

		ui32EDMDirListReg =
		    EUR_CR_BIF_DIR_LIST_BASE1 +
		    4 * (SGX_BIF_DIR_LIST_INDEX_EDM - 1);
#endif

		OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32EDMDirListReg,
			     psDevInfo->sKernelPDDevPAddr.uiAddr);
		PDUMPPDREGWITHFLAGS(ui32EDMDirListReg,
				    psDevInfo->sKernelPDDevPAddr.uiAddr,
				    ui32PDUMPFlags, PDUMP_PD_UNIQUETAG);
	}

	SGXResetInvalDC(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	PVR_DPF(PVR_DBG_MESSAGE, "Soft Reset of SGX");
	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	ui32RegVal = 0;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_SOFT_RESET, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_SOFT_RESET, ui32RegVal, ui32PDUMPFlags);

	SGXResetSleep(psDevInfo, ui32PDUMPFlags, IMG_TRUE);

	PDUMPCOMMENTWITHFLAGS(ui32PDUMPFlags, "End of SGX reset sequence\r\n");
}
