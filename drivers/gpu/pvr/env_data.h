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

#ifndef _ENV_DATA_
#define _ENV_DATA_

#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/workqueue.h>

#define PVRSRV_MAX_BRIDGE_IN_SIZE	0x1000
#define PVRSRV_MAX_BRIDGE_OUT_SIZE	0x1000

struct PVR_PCI_DEV {
	struct pci_dev *psPCIDev;
	enum HOST_PCI_INIT_FLAGS ePCIFlags;
	IMG_BOOL abPCIResourceInUse[DEVICE_COUNT_RESOURCE];
};

struct ENV_DATA {
	void *pvBridgeData;
	struct pm_dev *psPowerDevice;
	IMG_BOOL bLISRInstalled;
	IMG_BOOL bMISRInstalled;
	u32 ui32IRQ;
	void *pvISRCookie;
	struct workqueue_struct *psMISRWorkqueue;
	struct work_struct sMISRWork;
	void *pvSysData;
	struct workqueue_struct *psPerfWorkqueue;
	struct delayed_work sPerfWork;
};

#endif
