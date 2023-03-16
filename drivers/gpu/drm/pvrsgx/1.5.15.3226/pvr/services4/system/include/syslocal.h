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

#if !defined(__SYSLOCAL_H__)
#define __SYSLOCAL_H__

#define SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV		0x00000001UL
#define SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE	0x00000002UL
#define SYS_SPECIFIC_DATA_PCI_REQUEST_HOST_PORT_RANGE	0x00000004UL
#if defined(NO_HARDWARE)
#define SYS_SPECIFIC_DATA_ALLOC_DUMMY_SGX_REGS		0x00000008UL
#if defined(SUPPORT_MSVDX)
#define	SYS_SPECIFIC_DATA_ALLOC_DUMMY_MSVDX_REGS	0x00000020UL
#endif
#endif
#define	SYS_SPECIFIC_DATA_SGX_INITIALISED		0x00000040UL
//#if defined(SUPPORT_MSVDX)
#define	SYS_SPECIFIC_DATA_MSVDX_INITIALISED		0x00000080UL
//#endif
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


typedef struct _SYS_SPECIFIC_DATA_TAG_
{

	IMG_UINT32 ui32SysSpecificData;
#ifdef	__linux__
	PVRSRV_PCI_DEV_HANDLE hSGXPCI;
#endif
#ifdef	LDM_PCI
	struct pci_dev *psPCIDev;
#endif
#ifdef	SUPPORT_DRI_DRM_EXT

	IMG_UINT32 msi_addr;
	IMG_UINT32 msi_data;

	IMG_UINT32 saveBSM;
	IMG_UINT32 saveVBT;
#endif
} SYS_SPECIFIC_DATA;


#endif


