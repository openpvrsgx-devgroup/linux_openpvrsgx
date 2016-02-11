/*************************************************************************/ /*!
@Title          Utility defines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    This header provides system-specific declarations and macros
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

#if !defined(_SYS_UTILS_H_)
#define _SYS_UTILS_H_

#if defined(PDUMP)
/**
 * memory space name for device memory
 */
#define SYSTEM_PDUMP_NAME		"SYSMEM"
#endif

/**
 * System-specific flags indicating details about the devices
 */
#define SYS_SPECIFIC_DATA_DUMMY_SGX_REGS 			(0x00000001)
#define SYS_SPECIFIC_DATA_DUMMY_MSVDX_REGS 			(0x00000002)
#define SYS_SPECIFIC_DATA_ENVDATA 				(0x00000004)
#define SYS_SPECIFIC_DATA_LOCATEDEV 				(0x00000008)
#define SYS_SPECIFIC_DATA_REGSGXDEV				(0x00000010)
#define SYS_SPECIFIC_DATA_REGVXDDEV				(0x00000020)
#define SYS_SPECIFIC_DATA_INITSGXDEV				(0x00000040)
#define SYS_SPECIFIC_DATA_INITVXDDEV				(0x00000080)
#define SYS_SPECIFIC_DATA_MISR_INSTALLED			(0x00000100)
#define SYS_SPECIFIC_DATA_LISR_INSTALLED			(0x00000200)
#define SYS_SPECIFIC_DATA_IRQ_ENABLED				(0x00000400)
#define SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS			(0x00000800)
#define SYS_SPECIFIC_DATA_PM_UNMAP_MSVDX_REGS			(0x00001000)
#define SYS_SPECIFIC_DATA_PM_IRQ_DISABLE			(0x00002000)
#define	SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR			(0x00004000)
#define	SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV			(0x00008000)
#define	SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE		(0x00010000)
#define SYS_SPECIFIC_DATA_SOC_REGS_MAPPED			(0x00020000)

/*****************************************************************************
 * system specific configs
 *****************************************************************************/

typedef struct _SYS_SPECIFIC_DATA_TAG_
{
	IMG_UINT32 ui32SysSpecificData;

	PVRSRV_PCI_DEV_HANDLE hSGXPCI;

#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
	struct pci_dev *psPCIDev;
#endif

#if defined(SUPPORT_DRI_DRM)
	struct drm_device *psDRMDev;
#endif

#if defined(PVR_LINUX_USING_WORKQUEUES)
	struct mutex	sPowerLock;
#endif

} SYS_SPECIFIC_DATA;

/**
 * Device Map for the SOC registers
 */
typedef struct _CDV_DEVICE_MAP_
{
	/**
	 * The CPU virtual address for the SOC registers
	 */
	IMG_CPU_VIRTADDR sRegsCpuVBase;

	/**
	 * Size of the SOC register bank
	 */
	IMG_UINT32 ui32RegsSize;

	/**
	 * The CPU physical address of the SOC register bank
	 */
	IMG_CPU_PHYADDR sRegsCpuPBase;
	/**
	 * The system physical address of the SOC register bank
	 */
	IMG_SYS_PHYADDR sRegsSysPBase;


} CDV_DEVICE_MAP, *PCDV_DEVICE_MAP;

/**
 * Set a SYS_SPECIFIC flag
 */
#define	SYS_SPECIFIC_DATA_SET(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData |= (flag)))

/**
 * Clear a SYS_SPECIFIC flag
 */
#define	SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData &= ~(flag)))

/**
 * Test whether a SYS_SPECIFIC flag is set
 */
#define	SYS_SPECIFIC_DATA_TEST(psSysSpecData, flag) (((psSysSpecData)->ui32SysSpecificData & (flag)) != 0)

#define	CEDARVIEW_ADDR_RANGE_INDEX      (SYS_PCI_INDEX - 4)

/******************
 * Local function prototypes
 ******************/

PVRSRV_ERROR PCIInitDev(SYS_DATA *psSysData);
IMG_VOID PCIDeInitDev(SYS_DATA *psSysData);

PVRSRV_ERROR SysCreateVersionString(SYS_DATA *psSysData, SGX_DEVICE_MAP *psSGXDeviceMap);
IMG_VOID SysFreeVersionString(SYS_DATA *psSysData);

IMG_VOID SysEnableInterrupts(SYS_DATA *psSysData);
IMG_VOID SysDisableInterrupts(SYS_DATA *psSysData);

IMG_VOID SysPowerOnSGX(IMG_VOID);
IMG_VOID SysPowerOffSGX(IMG_VOID);

#if defined(PVR_LINUX_USING_WORKQUEUES)
IMG_VOID SysPowerMaybeInit(SYS_DATA *psSysData);
#else
static INLINE IMG_VOID SysPowerMaybeInit(SYS_DATA *psSysData)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
}
#endif

#endif /* !defined(_SYS_UTILS_H_) */
