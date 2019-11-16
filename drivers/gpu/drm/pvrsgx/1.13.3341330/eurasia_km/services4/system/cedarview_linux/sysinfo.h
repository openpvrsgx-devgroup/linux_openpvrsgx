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

#if !defined(__SYSINFO_H__)
#define __SYSINFO_H__

/**
 * Clock speed for SGX
 */
#define SYS_SGX_CLOCK_SPEED					(400000000)

/**
 * Hardware recovery checking frequency (?)
 */
#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100) /* 100hz */
/**
 * PDS timer frequency
 */
#define SYS_SGX_PDS_TIMER_FREQ				(1000) /* 1000hz */
/**
 * Time we allow devices to be idle before switching on active power management
 */
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(1)

/**
 * Maximum time we allow the hardware to respond in
 */
#define MAX_HW_TIME_US				(500000)
/**
 * Maximum number of times we try again before failing
 */
#define WAIT_TRY_COUNT				(10000)

/**
 * Device and Vendor ID
 */
#define SYS_SGX_DEV_VENDOR_ID		(0x8086)
#define SYS_SGX_DEV_DEVICE_ID		(0x0BE1)
#define SYS_SGX_DEV1_DEVICE_ID		(0x0BE2)

/**
 * Offset in UINT32s into PCI space to the
 * registers base physical address
 */
#define SYS_PCI_INDEX (0x4)

/**
 * Three devices in the system:
 * SGX
 * VXD
 * Display Controller
 */
#define SYS_DEVICE_COUNT (3)

#endif	/* __SYSINFO_H__ */
