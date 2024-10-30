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

#if !defined(__SYSLOCAL_H__)
#define __SYSLOCAL_H__

#define SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV		0x00000001
#define SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE	0x00000002
#define SYS_SPECIFIC_DATA_PCI_REQUEST_HOST_PORT_RANGE	0x00000004
#define	SYS_SPECIFIC_DATA_SGX_INITIALISED		0x00000040
#if defined(SUPPORT_MSVDX)
#define	SYS_SPECIFIC_DATA_MSVDX_INITIALISED		0x00000080
#endif	
#define	SYS_SPECIFIC_DATA_MISR_INSTALLED		0x00000100
#define	SYS_SPECIFIC_DATA_LISR_INSTALLED		0x00000200
#define	SYS_SPECIFIC_DATA_PDUMP_INIT			0x00000400
#define	SYS_SPECIFIC_DATA_IRQ_ENABLED			0x00000800

#define	SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS		0x00001000
#define	SYS_SPECIFIC_DATA_PM_UNMAP_SGX_HP		0x00004000
#define	SYS_SPECIFIC_DATA_PM_UNMAP_MSVDX_REGS		0x00008000
#define	SYS_SPECIFIC_DATA_PM_IRQ_DISABLE		0x00010000
#define	SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR		0x00020000

#define	SYS_SPECIFIC_DATA_SET(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData |= (flag)))

#define	SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData &= ~(flag)))

#define	SYS_SPECIFIC_DATA_TEST(psSysSpecData, flag) (((psSysSpecData)->ui32SysSpecificData & (flag)) != 0)
 
 
typedef struct _SYS_SPECIFIC_DATA_TAG_
{
	
	IMG_UINT32 ui32SysSpecificData;
#ifdef	__linux__
	PVRSRV_PCI_DEV_HANDLE hSGXPCI;
#endif
	struct pci_dev *psPCIDev;

	/* MSI reg save */
	uint32_t msi_addr;
	uint32_t msi_data;

	uint32_t saveBSM;
	uint32_t saveVBT;
} SYS_SPECIFIC_DATA;

 
#endif	


