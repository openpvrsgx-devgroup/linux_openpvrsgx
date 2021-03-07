/*************************************************************************/ /*!
@Title          System Description Header
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

#if !defined(__SOCCONFIG_H__)
#define __SOCCONFIG_H__

#define VS_PRODUCT_NAME	"Atlas PCI + SGX Test Chip 3"

#if defined(SGX535)
/* this system has a SGX host port */
#define SGX_FEATURE_HOST_PORT
#endif


#define PCI_BASEREG_OFFSET_DWORDS			4

/* Atlas reg on base register 0 */
#define SYS_ATLAS_REG_PCI_BASENUM		    0
#define SYS_ATLAS_REG_PCI_OFFSET		    (SYS_ATLAS_REG_PCI_BASENUM + PCI_BASEREG_OFFSET_DWORDS)

#define SYS_ATLAS_REG_OFFSET                0x0
#define SYS_ATLAS_REG_SIZE                  0x4000

#define SYS_PDP_REG_OFFSET                  0xC000
#define SYS_PDP_REG_SIZE                    0x4000

#define SYS_ATLAS_REG_REGION_SIZE			0x10000

/* SGX reg on base register 1 */
#define SYS_SGX_REG_PCI_BASENUM				1
#define SYS_SGX_REG_PCI_OFFSET		        (SYS_SGX_REG_PCI_BASENUM + PCI_BASEREG_OFFSET_DWORDS)

#define SYS_SGX_REG_OFFSET                  0x0

#if defined(SGX_FEATURE_MP)
/* One block for each core plus one for the all-cores bank and one for the master bank.*/
#define SYS_SGX_REG_SIZE 						(0x4000 * (SGX_FEATURE_MP_CORE_COUNT_3D + 2))
#else
#define SYS_SGX_REG_SIZE 						(0x4000)
#endif /* SGX_FEATURE_MP */

/* SGX mem (including HP mapping) on base register 2 */
#define SYS_SGX_MEM_PCI_BASENUM				2
#define SYS_SGX_MEM_PCI_OFFSET		        (SYS_SGX_MEM_PCI_BASENUM + PCI_BASEREG_OFFSET_DWORDS)

#define SYS_SGX_MEM_REGION_SIZE				0x20000000

/* Test Chip 3 ID */
#define SYS_SGX_DEV_VENDOR_ID		        0x1010
#define SYS_SGX_DEV_DEVICE_ID		        0x1CF1
#define SYS_SGX_DEV1_DEVICE_ID		        0x1CF2

// PCI card has 256mb limit we then take off 32mb for the PDP and 4mb for the buffer class.
#define SYS_SGX_DEV_LOCALMEM_FOR_SGX_RESERVE_SIZE   ((256 - 32 - 4) * 1024 * 1024)

// PCIe card has 512mb limit we then take off 32mb for the PDP and 4mb for the buffer class.
#define SYS_SGX_DEV1_LOCALMEM_FOR_SGX_RESERVE_SIZE   ((512 - 32 - 4) * 1024 * 1024)

/* Map no more than MEMTEST_MAP_SIZE into kernel space at any one time during the mem test */
#define MEMTEST_MAP_SIZE					(1024*1024)

#if defined(SGX_FEATURE_HOST_PORT)

	#define SYS_SGX_HP_SIZE						0x8000000

	/* 
		device virtual address of host port base
		actually 0x80000000 + HP offset within the BAR (256Mb)
	*/
	#define SYS_SGX_HOSTPORT_BASE_DEVVADDR 0x90000000
	
#endif

/* ISR Details for WinCE */

/*****************************************************************************
 * system specific configs
 *****************************************************************************/
 
 
#if !defined(__linux__)
/*!
 *****************************************************************************
	 PCI Config space mapping structures
 ****************************************************************************/
typedef struct  
{
	union
	{
		IMG_UINT8	aui8PCISpace[256];
		IMG_UINT16	aui16PCISpace[128];
		IMG_UINT32	aui32PCISpace[64];
		struct  
		{
			IMG_UINT16	ui16VenID;
			IMG_UINT16	ui16DevID;
			IMG_UINT16	ui16PCICmd;
			IMG_UINT16	ui16PCIStatus;
		}s;
	}u;

} PCICONFIG_SPACE, *PPCICONFIG_SPACE;
#endif

/*! Some other interesting registers */
#define SYS_SGX_DEV_ID_16_PCI_OFFSET   1
#define SYS_SGX_VEN_ID_16_PCI_OFFSET   0
#define SYS_SGX_SSDEV_ID_16_PCI_OFFSET 23
#define SYS_SGX_SSVEN_ID_16_PCI_OFFSET 22
#define SYS_SGX_IRQ_8_PCI_OFFSET       0x3C
#endif	/* __SOCCONFIG_H__ */

