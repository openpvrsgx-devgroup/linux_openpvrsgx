/*************************************************************************/ /*!
@Title          Local system definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    This header provides local system declarations and macros
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

#if !defined(__SYSLOCAL_H__)
#define __SYSLOCAL_H__

/*
 * Various flags to indicate what has been initialised.
 * Only used at driver initialisation/deinitialisation time.
 */

#define SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV		0x00000001UL
#define SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE	0x00000002UL
#define SYS_SPECIFIC_DATA_PCI_REQUEST_HOST_PORT_RANGE	0x00000004UL
#if defined(NO_HARDWARE)
#define SYS_SPECIFIC_DATA_ALLOC_DUMMY_SGX_REGS		0x00000008UL
#if defined(SUPPORT_MSVDX)
#define	SYS_SPECIFIC_DATA_ALLOC_DUMMY_MSVDX_REGS	0x00000020UL
#endif	/* defined(SUPPORT_MSVDX) */
#endif	/* defined(NO_HARDWARE) */
#define	SYS_SPECIFIC_DATA_SGX_INITIALISED		0x00000040UL
#if defined(SUPPORT_MSVDX)
#define	SYS_SPECIFIC_DATA_MSVDX_INITIALISED		0x00000080UL
#endif	/* defined(SUPPORT_MSVDX) */
#define	SYS_SPECIFIC_DATA_MISR_INSTALLED		0x00000100UL
#define	SYS_SPECIFIC_DATA_LISR_INSTALLED		0x00000200UL
#define	SYS_SPECIFIC_DATA_PDUMP_INIT			0x00000400UL
#define	SYS_SPECIFIC_DATA_IRQ_ENABLED			0x00000800UL

#define	SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS		0x00001000UL
#define	SYS_SPECIFIC_DATA_PM_UNMAP_SGX_HP		0x00004000UL
#define	SYS_SPECIFIC_DATA_PM_UNMAP_MSVDX_REGS		0x00008000UL
#define	SYS_SPECIFIC_DATA_PM_IRQ_DISABLE		0x00010000UL
#define	SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR		0x00020000UL

#define	SYS_SPECIFIC_DATA_SET(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData |= (flag)))

#define	SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData &= ~(flag)))

#define	SYS_SPECIFIC_DATA_TEST(psSysSpecData, flag) (((psSysSpecData)->ui32SysSpecificData & (flag)) != 0)
 
/*****************************************************************************
 * system specific data structures
 *****************************************************************************/
 
typedef struct _SYS_SPECIFIC_DATA_TAG_
{
	/* 
	 * Use the following field to record the process of the initialisation
	 * process, as in TC3 based systems.
	 */
	IMG_UINT32 ui32SysSpecificData;
#ifdef	__linux__
	PVRSRV_PCI_DEV_HANDLE hSGXPCI;
#endif
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

/*****************************************************************************
 * system specific function prototypes
 *****************************************************************************/

#if defined(PVR_LINUX_USING_WORKQUEUES)
IMG_VOID SysPowerMaybeInit(SYS_DATA *psSysData);
#else
static INLINE IMG_VOID SysPowerMaybeInit(SYS_DATA *psSysData)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
}
#endif

#endif	/* __SYSLOCAL_H__ */
