/*************************************************************************/ /*!
@Title          Shared (User/kernel) and System dependent utilities
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

#include "sgxdefs.h"
#include "services_headers.h"
#include "sysinfo.h"

#include "sgxinfo.h"

#include "tcfdefs.h"

#include "syslocal.h"



/* SYSTEM SPECIFIC FUNCTIONS */
/*!
******************************************************************************

 @Function	SysInitRegisters

 @Description programs some sets up registers for the device

 @Input    sRegRegion - CPU phys addr of register region

 @Return   PVRSRV_ERROR  :

******************************************************************************/
PVRSRV_ERROR SysInitRegisters(IMG_VOID)
{
	SYS_DATA	*psSysData;
	IMG_VOID	*pvAtlasRegsLinAddr;
	IMG_VOID	*pvPDPRegsLinAddr;

	SysAcquireData(&psSysData);

	pvAtlasRegsLinAddr = psSysData->pvSOCRegsBase;

	if (pvAtlasRegsLinAddr)
	{
		IMG_UINT32 ui32AtlasReset;
		IMG_UINT32 ui32FPGA_Id;

		pvPDPRegsLinAddr = (IMG_PBYTE)pvAtlasRegsLinAddr - SYS_ATLAS_REG_OFFSET + SYS_PDP_REG_OFFSET;

        ui32FPGA_Id = SysReadHWReg(pvAtlasRegsLinAddr, TCF_CR_FPGA_ID_REG);
		if (((ui32FPGA_Id & TCF_CR_FPGA_REV_REG_FPGA_REV_REG_DESIGNER_MASK) !=
				(0x12U << TCF_CR_FPGA_REV_REG_FPGA_REV_REG_DESIGNER_SHIFT)) ||
			((ui32FPGA_Id & TCF_CR_FPGA_REV_REG_FPGA_REV_REG_MAJOR_MASK) <
				(SGX_ATLAS_FPGA_REV << TCF_CR_FPGA_REV_REG_FPGA_REV_REG_MAJOR_SHIFT)))
		{
			PVR_DPF((PVR_DBG_ERROR, "SysInitRegisters: TCF_CR_FPGA_ID_REG mismatch %x", SysReadHWReg(pvAtlasRegsLinAddr, TCF_CR_FPGA_ID_REG)));
			return PVRSRV_ERROR_NOT_SUPPORTED;
		}

		ui32AtlasReset = SysReadHWReg(pvAtlasRegsLinAddr, TCF_CR_CLK_AND_RST_CTRL);

#if defined(SGX_APOLLO_FPGA_REV)
		ui32AtlasReset &= ~TCF_CR_CLK_AND_RST_CTRL_DUT_RESETN_MASK;
		ui32AtlasReset &= ~TCF_CR_CLK_AND_RST_CTRL_DDR_RESETN_MASK;
		ui32AtlasReset &= ~TCF_CR_CLK_AND_RST_CTRL_DUT_DCM_RESETN_MASK;
		SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_CLK_AND_RST_CTRL, ui32AtlasReset);
		OSWaitus(100);
		ui32AtlasReset |= TCF_CR_CLK_AND_RST_CTRL_DDR_RESETN_MASK;
		SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_CLK_AND_RST_CTRL, ui32AtlasReset);
		OSWaitus(1000);
		ui32AtlasReset |= TCF_CR_CLK_AND_RST_CTRL_DUT_RESETN_MASK;
		SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_CLK_AND_RST_CTRL, ui32AtlasReset);
		OSWaitus(1000);
		ui32AtlasReset |= TCF_CR_CLK_AND_RST_CTRL_DUT_DCM_RESETN_MASK;
		SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_CLK_AND_RST_CTRL, ui32AtlasReset);
		OSWaitus(1000);

        /* Sense the interrupt output on active high */
        ui32AtlasReset = SysReadHWReg(pvAtlasRegsLinAddr, TCF_CR_INTERRUPT_OP_CFG);
        ui32AtlasReset &= ~TCF_CR_INTERRUPT_OP_CFG_INT_SENSE_MASK;
        SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_INTERRUPT_OP_CFG, ui32AtlasReset);
        OSWaitus(1000);

		/* Magic to turn switch from I2P board to VGA video board */
		SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_EXP_BRD_CTRL, 0x702);
#else /* defined(SGX_APOLLO_FPGA_REV) */
        /* Reset magic PLL thing from Atlas */
		SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_CLK_AND_RST_CTRL, ui32AtlasReset & ~TCF_CR_CLK_AND_RST_CTRL_DCM_RESETN_MASK);

		OSWaitus(100);

		SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_CLK_AND_RST_CTRL, ui32AtlasReset | TCF_CR_CLK_AND_RST_CTRL_DCM_RESETN_MASK);
   
		OSWaitus(1000);

        /* Reset SGX from Atlas */
		SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_CLK_AND_RST_CTRL, ui32AtlasReset & ~TCF_CR_CLK_AND_RST_CTRL_SGX535_RESETN_MASK);

		OSWaitus(100);

		SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_CLK_AND_RST_CTRL, ui32AtlasReset | TCF_CR_CLK_AND_RST_CTRL_SGX535_RESETN_MASK);

		OSWaitus(1000);

		/* Setup DDR */
        if(SysReadHWReg(pvAtlasRegsLinAddr, TCF_CR_RFEN_REG) != 0x82360035)
        {
            IMG_UINT32 ui32PDPStatus;

            PVR_DPF((PVR_DBG_ERROR,"Writing TEST_CHIP DDR Setup registers"));

            /* First make sure we disable the PDP while we setup the DDR */
            ui32PDPStatus = SysReadHWReg(pvPDPRegsLinAddr, 0x4);

            if(ui32PDPStatus == 0x00000000)
            {
                SysWriteHWReg(pvPDPRegsLinAddr, 0x4, 0x00000000);
                OSWaitus(1000);
            }

            SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_RFEN_REG, 0x82360035);
            OSWaitus(100);
            SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_MEM_CONTROL, 0x2A520B54);
            OSWaitus(100);
            SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_MEM_CONTROL, 0x2A520B55);
            OSWaitus(100);
            SysWriteHWReg(pvAtlasRegsLinAddr, TCF_CR_MEM_CONTROL, 0x2A520B54);
            OSWaitus(1000);

            if(ui32PDPStatus != 0x00000000)
            {
                SysWriteHWReg(pvPDPRegsLinAddr, 0x4, ui32PDPStatus);
                OSWaitus(1000);
            }
        }

		/* Magic to turn switch from I2P board to VGA video board */
		SysWriteHWReg(pvAtlasRegsLinAddr, 0x110, 0x702);
#endif /* defined(SGX_APOLLO_FPGA_REV) */
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "SysInitRegisters: Register base not set"));
		return PVRSRV_ERROR_REGISTER_BASE_NOT_SET;
	}

	return PVRSRV_OK;
}


IMG_CHAR *SysCreateVersionString(IMG_CPU_PHYADDR sRegRegion)
{
    static IMG_CHAR aszVersionString[100];
#if !defined(NO_HARDWARE)
    IMG_VOID	*pvRegsLinAddr;
#endif
    SYS_DATA	*psSysData;
    IMG_UINT32	ui32SGXRevision;
    IMG_UINT32	ui32AtlasRevision;
    IMG_UINT32	ui32TCFRevision;
    IMG_INT32	i32Count;

#if !defined(NO_HARDWARE)
    pvRegsLinAddr = OSMapPhysToLin(sRegRegion,
                                   SYS_SGX_REG_SIZE,
                                   PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
                                   IMG_NULL);
    if(!pvRegsLinAddr)
    {
		return IMG_NULL;
    }

    ui32SGXRevision = OSReadHWReg((IMG_PVOID)((IMG_PBYTE)pvRegsLinAddr + SYS_SGX_REG_OFFSET),
                                      EUR_CR_CORE_REVISION);
#else
    PVR_UNREFERENCED_PARAMETER(sRegRegion);
    ui32SGXRevision = 0;
#endif

    SysAcquireData(&psSysData);

    ui32AtlasRevision = SysReadHWReg(psSysData->pvSOCRegsBase, TCF_CR_FPGA_REV_REG);
    ui32TCFRevision = SysReadHWReg(psSysData->pvSOCRegsBase, TCF_CR_TCF_CORE_REV_REG);

	/* SGX544 TC reports incorrect core revision: 1.0.3. Correct value is 1.0.4.
	 * This is used to generate string in /proc so doesn't affect compatibility check.
	 */
    i32Count = OSSNPrintf(aszVersionString, 100,
                           "SGX revision = %u.%u.%u, Atlas revision = %u.%u.%u, TCF core revision = %u.%u.%u",
                           (IMG_UINT)((ui32SGXRevision & EUR_CR_CORE_REVISION_MAJOR_MASK)
                            >> EUR_CR_CORE_REVISION_MAJOR_SHIFT),
                           (IMG_UINT)((ui32SGXRevision & EUR_CR_CORE_REVISION_MINOR_MASK)
                            >> EUR_CR_CORE_REVISION_MINOR_SHIFT),
#if !defined(SGX544)
                           (IMG_UINT)((ui32SGXRevision & EUR_CR_CORE_REVISION_MAINTENANCE_MASK)
                            >> EUR_CR_CORE_REVISION_MAINTENANCE_SHIFT),
#else
			   4,
#endif
                           (IMG_UINT)((ui32AtlasRevision & TCF_CR_FPGA_REV_REG_FPGA_REV_REG_MAJOR_MASK)
                            >> TCF_CR_FPGA_REV_REG_FPGA_REV_REG_MAJOR_SHIFT),
                           (IMG_UINT)((ui32AtlasRevision & TCF_CR_FPGA_REV_REG_FPGA_REV_REG_MINOR_MASK)
                            >> TCF_CR_FPGA_REV_REG_FPGA_REV_REG_MINOR_SHIFT),
                           (IMG_UINT)((ui32AtlasRevision & TCF_CR_FPGA_REV_REG_FPGA_REV_REG_MAINT_MASK)
                            >> TCF_CR_FPGA_REV_REG_FPGA_REV_REG_MAINT_SHIFT),
                           (IMG_UINT)((ui32TCFRevision & TCF_CR_TCF_CORE_REV_REG_TCF_CORE_REV_REG_MAJOR_MASK)
                            >> TCF_CR_TCF_CORE_REV_REG_TCF_CORE_REV_REG_MAJOR_SHIFT),
                           (IMG_UINT)((ui32TCFRevision & TCF_CR_TCF_CORE_REV_REG_TCF_CORE_REV_REG_MINOR_MASK)
                            >> TCF_CR_TCF_CORE_REV_REG_TCF_CORE_REV_REG_MINOR_SHIFT),
                           (IMG_UINT)((ui32TCFRevision & TCF_CR_TCF_CORE_REV_REG_TCF_CORE_REV_REG_MAINT_MASK)
                            >> TCF_CR_TCF_CORE_REV_REG_TCF_CORE_REV_REG_MAINT_SHIFT)
                           );

#if !defined(NO_HARDWARE)
    OSUnMapPhysToLin(pvRegsLinAddr,
                     SYS_SGX_REG_SIZE,
                     PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
                     IMG_NULL);
#endif

	if(i32Count == -1)
    {
        return IMG_NULL;
    }

    return aszVersionString;
}


IMG_VOID SysResetSGX(IMG_PVOID pvRegsBaseKM)
{
	IMG_UINT32 ui32RegVal;

	/* Reset SGX from Atlas */
	ui32RegVal = SysReadHWReg(pvRegsBaseKM, TCF_CR_CLK_AND_RST_CTRL);
#if defined(SGX_APOLLO_FPGA_REV)
	SysWriteHWReg(pvRegsBaseKM, TCF_CR_CLK_AND_RST_CTRL,
				 ui32RegVal & ~TCF_CR_CLK_AND_RST_CTRL_DUT_RESETN_MASK);
#else /* defined(SGX_APOLLO_FPGA_REV) */
	SysWriteHWReg(pvRegsBaseKM, TCF_CR_CLK_AND_RST_CTRL,
				 ui32RegVal & ~TCF_CR_CLK_AND_RST_CTRL_SGX535_RESETN_MASK);
#endif /* defined(defined(SGX_APOLLO_FPGA_REV) */

	OSWaitus(100);
	SysWriteHWReg(pvRegsBaseKM, TCF_CR_CLK_AND_RST_CTRL,
				 ui32RegVal);
	OSWaitus(100);
}


#if defined(READ_TCF_HOST_MEM_SIGNATURE)

#include <stdlib.h>

/*!
******************************************************************************

 @Function	SysResetTCFHostMemSig

 @Description resets the TCF host memory signature register

 @Input    pvAtlasRegsBaseKM - CPU lin addr of register region

 @Return   PVRSRV_ERROR  :

******************************************************************************/
PVRSRV_ERROR SysResetTCFHostMemSig(IMG_PVOID pvAtlasRegsBaseKM)
{
	IMG_UINT32 ui32RegVal;

	SysWriteHWReg(pvAtlasRegsBaseKM, TCF_CR_CLEAR_HOST_MEM_SIG, 1);
	OSWaitus(100);

	SysWriteHWReg(pvAtlasRegsBaseKM, TCF_CR_CLEAR_HOST_MEM_SIG, 0);
	OSWaitus(100);

	ui32RegVal = SysReadHWReg(pvAtlasRegsBaseKM, TCF_CR_HOST_MEM_SIGNATURE);

	while(ui32RegVal != 0)
	{
		PVR_DPF((PVR_DBG_ERROR,"**** Atlas sig reset failed: %x ****", ui32RegVal));

		SysWriteHWReg(pvAtlasRegsBaseKM, TCF_CR_CLEAR_HOST_MEM_SIG, 1);
		OSWaitus(100);
		SysWriteHWReg(pvAtlasRegsBaseKM, TCF_CR_CLEAR_HOST_MEM_SIG, 0);
		OSWaitus(100);

		ui32RegVal = SysReadHWReg(pvAtlasRegsBaseKM, TCF_CR_HOST_MEM_SIGNATURE);
	}

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	SysReadTCFHostMemSig

 @Description reads the TCF CRC register, then resets it

 @Input    pvAtlasRegsBaseKM - CPU lin addr of register region

 @Return   PVRSRV_ERROR  :

******************************************************************************/
PVRSRV_ERROR SysReadTCFHostMemSig(IMG_PVOID pvAtlasRegsBaseKM)
{
	static IMG_UINT32 uCount = 0;
	IMG_UINT32 ui32RegVal;

	/* Wait a while for writes to flush */
	OSWaitus(1000);

	ui32RegVal = SysReadHWReg(pvAtlasRegsBaseKM, TCF_CR_HOST_MEM_SIGNATURE);

	PVR_DPF((PVR_DBG_ERROR,"**** TCF Signature read %d = 0x%x ****\n", uCount++, ui32RegVal));

	SysResetTCFHostMemSig(pvAtlasRegsBaseKM);

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	SysTestTCFHostMemSig

 @Description performs a series of memory writes and reads the TCF CRC register

 @Input     sRegRegion - CPU phys addr of register region
			sMemRegion - CPU phys addr of memory region

 @Return   PVRSRV_ERROR	 :

******************************************************************************/
PVRSRV_ERROR SysTestTCFHostMemSig(IMG_CPU_PHYADDR sRegRegion,
								  IMG_CPU_PHYADDR sMemRegion)
{
	SYS_DATA	*psSysData;
	IMG_UINT32 *puLinAddr;
	IMG_UINT8 *puWrite8;
	IMG_UINT16 *puWrite16;
	IMG_UINT32 *puWrite32;
	IMG_UINT32 i, offset, size;

	PVR_UNREFERENCED_PARAMETER(sRegRegion);

#define MEM_TEST_SIZE_IN_BYTES 0x100000

	SysAcquireData(&psSysData);

#if 1
	puLinAddr = OSMapPhysToLin(sMemRegion,
                               MEM_TEST_SIZE_IN_BYTES,
                               PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
                               IMG_NULL);
#else
	puLinAddr = OSMapPhysToLin(sMemRegion,
                               MEM_TEST_SIZE_IN_BYTES,
                               PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
                               IMG_NULL);
#endif

	if(!puLinAddr)
	{
		return PVRSRV_ERROR_BAD_MAPPING;
	}

	puWrite8  = (IMG_UINT8 *)puLinAddr;
	puWrite16 = (IMG_UINT16 *)puLinAddr;
	puWrite32 = puLinAddr;

	/* Reset the CRC before we start */
	SysResetTCFHostMemSig(psSysData->pvSOCRegsBase);

	/* Initialise random number seed */
	srand(0xdeadbeef);

	PVR_DPF((PVR_DBG_ERROR,"#### SysTestTCFHostMemSig start ####"));

	/* Do some random writes */
	for(i=0; i<0xFFFFF; i++)
	{
		offset = rand();
		offset = offset & (MEM_TEST_SIZE_IN_BYTES - 1);

		size = rand();
		size = size & 3;

		switch(size)
		{
			case 0:
				puWrite8[offset] = (IMG_UINT8)rand();
			break;

			case 1:
				puWrite16[offset>>1] = (IMG_UINT16)rand();
			break;

			case 2:
			case 3:
				puWrite32[offset>>2] = rand();
			break;
		}
	}

	/* Do some random reads to flush out the data */
	for(i=0; i<0xFF; i++)
	{
		IMG_UINT32 ui32ReadVal;

		offset = rand();
		offset = offset & (MEM_TEST_SIZE_IN_BYTES - 1);

		size = rand();
		size = size & 3;

		switch(size)
		{
			case 0:
				ui32ReadVal = (IMG_UINT32)puWrite8[offset];
			break;

			case 1:
				ui32ReadVal = (IMG_UINT32)puWrite16[offset>>1];
			break;

			case 2:
			case 3:
				ui32ReadVal = (IMG_UINT32)puWrite32[offset>>2];
			break;
		}
	}

	SysReadTCFHostMemSig(psSysData->pvSOCRegsBase);

	/* Unmap the memory */
#if 1
	OSUnMapPhysToLin(puLinAddr,
                     MEM_TEST_SIZE_IN_BYTES,
                     PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
                     IMG_NULL);
#else
	OSUnMapPhysToLin(puLinAddr,
                     MEM_TEST_SIZE_IN_BYTES,
                     PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
                     IMG_NULL);
#endif
	PVR_DPF((PVR_DBG_ERROR,"#### SysTestTCFHostMemSig end ####"));

	return PVRSRV_OK;
}

#endif /* READ_TCF_HOST_MEM_SIGNATURE */


/*!
******************************************************************************

 @Function	SysGet[Device]InterruptBit

 @Description 	Returns the SOC interrupt bit for the device

 @Return   IMG_UINT32

******************************************************************************/
IMG_UINT32 SysGetSGXInterruptBit(IMG_VOID)
{
#if defined(SGX_APOLLO_FPGA_REV)
	return TCF_CR_INTERRUPT_STATUS_EXT_INT_MASK;
#else
	return TCF_CR_INTERRUPT_STATUS_EXT_MASK;
#endif /* defined(SGX_APOLLO_FPGA_REV) */
}

IMG_UINT32 SysGetPDPInterruptBit(IMG_VOID)
{
#if defined(SGX_APOLLO_FPGA_REV)
	return TCF_CR_INTERRUPT_STATUS_PDP1_INT_MASK;
#else
	return TCF_CR_INTERRUPT_STATUS_PDP1_MASK;
#endif /* defined(SGX_APOLLO_FPGA_REV) */

}


/*!
******************************************************************************

 @Function	SysEnableInterrupts

 @Description 	Enables system interrupts

 @Input    pointer to system specific data

 @Return   none

******************************************************************************/
IMG_VOID SysEnableInterrupts(SYS_DATA *psSysData)
{
	IMG_UINT32 ui32InterruptEnable = SysReadHWReg(psSysData->pvSOCRegsBase, TCF_CR_INTERRUPT_ENABLE);
#if defined(SGX_APOLLO_FPGA_REV)
	ui32InterruptEnable |= TCF_CR_INTERRUPT_ENABLE_EXT_INT_MASK | TCF_CR_INTERRUPT_ENABLE_PDP1_INT_MASK;
#else
	ui32InterruptEnable |= TCF_CR_INTERRUPT_ENABLE_EXT_ENABLE | TCF_CR_INTERRUPT_ENABLE_PDP1_ENABLE;
#endif /* defined(SGX_APOLLO_FPGA_REV) */
	SysWriteHWReg(psSysData->pvSOCRegsBase, TCF_CR_INTERRUPT_ENABLE, ui32InterruptEnable);
}


/*!
******************************************************************************

 @Function	SysDisableInterrupts

 @Description 	Disables all system interrupts

 @Input    pointer to system specific data

 @Return   none

******************************************************************************/
IMG_VOID SysDisableInterrupts(SYS_DATA *psSysData)
{
	IMG_UINT32 ui32InterruptEnable;

	if (psSysData->pvSOCRegsBase)
	{
		ui32InterruptEnable = SysReadHWReg(psSysData->pvSOCRegsBase, TCF_CR_INTERRUPT_ENABLE);
#if defined(SGX_APOLLO_FPGA_REV)
		ui32InterruptEnable &= ~(TCF_CR_INTERRUPT_ENABLE_EXT_INT_MASK | TCF_CR_INTERRUPT_ENABLE_PDP1_INT_MASK);
#else
		ui32InterruptEnable &= ~(TCF_CR_INTERRUPT_ENABLE_EXT_ENABLE | TCF_CR_INTERRUPT_ENABLE_PDP1_ENABLE);
#endif /* defined(SGX_APOLLO_FPGA_REV) */
		SysWriteHWReg(psSysData->pvSOCRegsBase, TCF_CR_INTERRUPT_ENABLE, ui32InterruptEnable);
	}
}


/*!
******************************************************************************

 @Function      SysGetInterruptSource

 @Description 	Returns System specific information about the device(s) that
                generated the interrupt in the system

 @Input         psSysData
 @Input         psDeviceNode - always NULL for a system ISR

 @Return        System specific information indicating which device(s)
                generated the interrupt

******************************************************************************/
IMG_UINT32 SysGetInterruptSource(SYS_DATA			*psSysData,
								 PVRSRV_DEVICE_NODE *psDeviceNode)
{
	IMG_UINT32 ui32IntEnable, ui32IntStatus, ui32IntProc;

	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_ASSERT(psDeviceNode == IMG_NULL);

	ui32IntEnable = SysReadHWReg(psSysData->pvSOCRegsBase, TCF_CR_INTERRUPT_ENABLE);
	ui32IntStatus = SysReadHWReg(psSysData->pvSOCRegsBase, TCF_CR_INTERRUPT_STATUS);
	ui32IntProc = ui32IntStatus & ui32IntEnable;

	return ui32IntProc;
}


/*!
******************************************************************************

 @Function      SysClearInterrupts

 @Description 	Clears specified system interrupts

 @Input         psSysData
 @Input         ui32InterruptBits

 @Return        none

******************************************************************************/
IMG_VOID SysClearInterrupts(SYS_DATA	*psSysData,
							IMG_UINT32	ui32InterruptBits)
{
	IMG_UINT32 ui32IntClear = 0;

#if defined(SGX_APOLLO_FPGA_REV)
	if (ui32InterruptBits & TCF_CR_INTERRUPT_STATUS_EXT_INT_MASK)
	{
		ui32IntClear |= TCF_CR_INTERRUPT_CLEAR_EXT_INT_MASK;
	}
	if (ui32InterruptBits & TCF_CR_INTERRUPT_STATUS_PDP1_INT_MASK)
	{
		ui32IntClear |= TCF_CR_INTERRUPT_CLEAR_PDP1_INT_MASK;
	}
#else
	if (ui32InterruptBits & TCF_CR_INTERRUPT_STATUS_EXT_MASK)
	{
		ui32IntClear |= TCF_CR_INTERRUPT_CLEAR_EXT_MASK;
	}
	if (ui32InterruptBits & TCF_CR_INTERRUPT_STATUS_PDP1_MASK)
	{
		ui32IntClear |= TCF_CR_INTERRUPT_CLEAR_PDP1_MASK;
	}
#endif /* defined(SGX_APOLLO_FPGA_REV) */

	SysWriteHWReg(psSysData->pvSOCRegsBase, TCF_CR_INTERRUPT_CLEAR, ui32IntClear);
}



