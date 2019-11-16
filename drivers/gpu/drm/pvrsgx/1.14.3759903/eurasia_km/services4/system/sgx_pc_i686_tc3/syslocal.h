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

/*****************************************************************************
 * system specific data structures
 *****************************************************************************/
 
/*****************************************************************************
 * system specific function prototypes
 *****************************************************************************/
 
IMG_VOID SysEnableInterrupts(SYS_DATA *psSysData);
IMG_VOID SysDisableInterrupts(SYS_DATA *psSysData);
IMG_UINT32 SysGetSGXInterruptBit(IMG_VOID);
IMG_UINT32 SysGetPDPInterruptBit(IMG_VOID);

IMG_CHAR *SysCreateVersionString(IMG_CPU_PHYADDR sRegRegion);

/* Various flags to track driver initialisation state */

#define SYS_SPECIFIC_DATA_ENABLE_IRQ		0x00000001UL
#define SYS_SPECIFIC_DATA_ENABLE_LISR		0x00000002UL
#define SYS_SPECIFIC_DATA_ENABLE_MISR		0x00000004UL
#define SYS_SPECIFIC_DATA_ENABLE_ENVDATA	0x00000008UL
#define SYS_SPECIFIC_DATA_ENABLE_PCINIT		0x00000010UL
#define SYS_SPECIFIC_DATA_ENABLE_LOCATEDEV	0x00000020UL
#define SYS_SPECIFIC_DATA_ENABLE_RA_ARENA	0x00000040UL
#define SYS_SPECIFIC_DATA_ENABLE_INITREG	0x00000080UL
#define SYS_SPECIFIC_DATA_ENABLE_REGDEV		0x00000100UL
#define SYS_SPECIFIC_DATA_ENABLE_PDUMPINIT	0x00000200UL
#define SYS_SPECIFIC_DATA_ENABLE_INITDEV	0x00000400UL
#define SYS_SPECIFIC_DATA_ENABLE_PCI_MEM	0x00000800UL
#define SYS_SPECIFIC_DATA_ENABLE_PCI_REG	0x00001000UL
#define SYS_SPECIFIC_DATA_ENABLE_PCI_ATL	0x00002000UL
#define SYS_SPECIFIC_DATA_ENABLE_PCI_DEV	0x00004000UL

#if defined (NO_HARDWARE)
#define SYS_SPECIFIC_DATA_ENABLE_REG_MEM	0x00008000UL
#endif

#define	SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS	0x00020000UL
#define	SYS_SPECIFIC_DATA_PM_UNMAP_SGX_HP	0x00080000UL
#define	SYS_SPECIFIC_DATA_PM_IRQ_DISABLE	0x00100000UL
#define	SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR	0x00200000UL

#define	SYS_SPECIFIC_DATA_SET(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData |= (flag)))

#define	SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData &= ~(flag)))

#define	SYS_SPECIFIC_DATA_TEST(psSysSpecData, flag) (((psSysSpecData)->ui32SysSpecificData & (flag)) != 0)
 
typedef struct _SYS_SPECIFIC_DATA_TAG_
{
	IMG_UINT32 ui32SysSpecificData;
#ifdef	__linux__
	PVRSRV_PCI_DEV_HANDLE hSGXPCI;
#endif
#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
	struct pci_dev *psPCIDev;
#endif
} SYS_SPECIFIC_DATA;

#endif	/* __SYSLOCAL_H__ */


