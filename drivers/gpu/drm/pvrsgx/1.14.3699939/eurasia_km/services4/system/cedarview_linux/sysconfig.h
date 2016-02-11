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

#if !defined(__SYSCONFIG_H__)
#define __SYSCONFIG_H__

#define VS_PRODUCT_NAME	"SGX Cedarview"

/**************
 * SGX Defines
 **************/
#define SYS_SGX_REG_OFFSET		(0x80000) /* Max offset is 0x9FFFF */
#define SYS_SGX_REG_SIZE		(0x4000)

/**************
 * VXD Defines
 **************/
#define SYS_MSVDX_REG_OFFSET		(0x90000) /* Max offset is 0x8FFFF */
#define SYS_MSVDX_REG_SIZE			(MSVDX_REG_SIZE)

/**************
 * Display Defines
 **************/
#define SYS_DISPLAY_REG_OFFSET		(0x05000) /* Max offset is 0x7FFFF */
/* Don't need the display size, as we never emulate these */

#define DEVICE_SGX_INTERRUPT		(1<<0)
#define DEVICE_MSVDX_INTERRUPT		(1<<1)
#define DEVICE_DISP_INTERRUPT		(1<<2)

#define SYS_SOC_REG_OFFSET					(0x0)
#define SYS_SOC_REG_SIZE					(0x2100)
/*
 * System interrupt status registers
 */
#define CDV_INTERRUPT_ENABLE_REG		0x20A0
#define CDV_INTERRUPT_IDENTITY_REG		0x20A4
#define CDV_INTERRUPT_MASK_REG			0x20A8
#define CDV_INTERRUPT_STATUS_REG		0x20AC

/*
 * Bits within these registers giving details about which
 * bit of hardware has generated the interrupt
 */
#define CDV_SGX_MASK					(1UL<<18)
#define CDV_MSVDX_MASK					(1UL<<19)
/* All other bits are display-related, and are handled by the DIM internally,
 * so we don't need to know them here */
#define CDV_VSYNC_PIPEA_VBLANK_MASK	(1UL<<7)
#define CDV_VSYNC_PIPEA_EVENT_MASK		(1UL<<6)
#define CDV_VSYNC_PIPEB_VBLANK_MASK	(1UL<<5)
#define CDV_VSYNC_PIPEB_EVENT_MASK		(1UL<<4)

/**************
 * SOC Defines
 **************/

#endif	/* __SYSCONFIG_H__ */
