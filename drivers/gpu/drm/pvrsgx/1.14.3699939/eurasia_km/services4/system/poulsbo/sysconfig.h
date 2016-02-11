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

#define VS_PRODUCT_NAME	"SGX Poulsbo"

/* this system has a SGX host port */
#define SGX_FEATURE_HOST_PORT

#define POULSBO_REGS_OFFSET	0x00000
#define POULSBO_REG_SIZE	0x2100	/* will cover Interrupt registers */

#define SGX_REGS_OFFSET		0x40000
#define SGX_REG_SIZE 		0x4000
#define MSVDX_REGS_OFFSET	0x50000

#ifdef SUPPORT_MSVDX
#define	POULSBO_MAX_OFFSET	(MSVDX_REGS_OFFSET + MSVDX_REG_SIZE)
#else
#define	POULSBO_MAX_OFFSET	(SGX_REGS_OFFSET + SGX_REG_SIZE)
#endif

/* Poulsbo SGX PCI Ids */
#define SYS_SGX_DEV_VENDOR_ID		0x8086
#define SYS_SGX_DEV_DEVICE_ID		0x8108

#define MMADR_INDEX			4
#define IOPORT_INDEX		5
#define GMADR_INDEX			6
#define MMUADR_INDEX		7
#define FBADR_INDEX			23
#define FBSIZE_INDEX		24

#define DISPLAY_SURFACE_SIZE        (4 * 1024 * 1024)

#define DEVICE_SGX_INTERRUPT		(1UL<<0)
#define DEVICE_MSVDX_INTERRUPT		(1UL<<1)
#define DEVICE_DISP_INTERRUPT		(1UL<<2)

/* Poulsbo Interrupt register defines */

#define POULSBO_INTERRUPT_ENABLE_REG		0x20A0
#define POULSBO_INTERRUPT_IDENTITY_REG		0x20A4
#define POULSBO_INTERRUPT_MASK_REG			0x20A8
#define POULSBO_INTERRUPT_STATUS_REG		0x20AC

#define POULSBO_DISP_MASK					(1UL<<17)
#define POULSBO_THALIA_MASK					(1UL<<18)
#define POULSBO_MSVDX_MASK					(1UL<<19)
#define POULSBO_VSYNC_PIPEA_VBLANK_MASK		(1UL<<7)
#define POULSBO_VSYNC_PIPEA_EVENT_MASK		(1UL<<6)
#define POULSBO_VSYNC_PIPEB_VBLANK_MASK		(1UL<<5)
#define POULSBO_VSYNC_PIPEB_EVENT_MASK		(1UL<<4)

#define POULSBO_DISPLAY_REGS_OFFSET			0x70000
#define POULSBO_DISPLAY_REG_SIZE			0x2000		/* will cover sync registers */

#define POULSBO_DISPLAY_A_CONFIG			0x00008UL
#define POULSBO_DISPLAY_A_STATUS_SELECT		0x00024UL
#define POULSBO_DISPLAY_B_CONFIG			0x01008UL
#define POULSBO_DISPLAY_B_STATUS_SELECT		0x01024UL

#define POULSBO_DISPLAY_PIPE_ENABLE			(1UL<<31)
#define POULSBO_DISPLAY_VSYNC_STS_EN		(1UL<<25)
#define POULSBO_DISPLAY_VSYNC_STS			(1UL<<9)

#if defined(SGX_FEATURE_HOST_PORT)
	#define SYS_SGX_HP_SIZE		0x8000000
	/* device virtual address of host port base */
	#define SYS_SGX_HOSTPORT_BASE_DEVVADDR 0xD0000000
#endif

/* ISR Details for WinCE */

/*****************************************************************************
 * system specific data structure
 *****************************************************************************/
 
/*!
 *****************************************************************************
	 PCI Config space mapping structure
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

#endif	/* __SOCCONFIG_H__ */
