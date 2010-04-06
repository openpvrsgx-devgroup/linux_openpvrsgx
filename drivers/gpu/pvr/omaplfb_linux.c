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

#include <linux/version.h>
#include <linux/module.h>

#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/interrupt.h>

#include <linux/platform_device.h>

#include <linux/io.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "omaplfb.h"
#include "pvrmodule.h"

#include <plat/display.h>

MODULE_SUPPORTED_DEVICE(DEVNAME);

#define unref__ __attribute__ ((unused))

void *OMAPLFBAllocKernelMem(u32 ui32Size)
{
	return kmalloc(ui32Size, GFP_KERNEL);
}

void OMAPLFBFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

enum PVRSRV_ERROR OMAPLFBGetLibFuncAddr(char *szFunctionName,
	       IMG_BOOL (**ppfnFuncTable)(struct PVRSRV_DC_DISP2SRV_KMJTABLE *))
{
	if (strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
		return PVRSRV_ERROR_INVALID_PARAMS;

	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return PVRSRV_OK;
}

static int OMAPLFBDriverSuspend_Entry(struct platform_device unref__ * pDevice,
				      pm_message_t unref__ state)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		      ": OMAPLFBDriverSuspend_Entry\n"));
	return 0;
}

static int OMAPLFBDriverResume_Entry(struct platform_device unref__ * pDevice)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": OMAPLFBDriverResume_Entry\n"));
	return 0;
}

static void OMAPLFBDriverShutdown_Entry(struct platform_device unref__ *
					pDevice)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		      ": OMAPLFBDriverShutdown_Entry\n"));
}

static void OMAPLFBDeviceRelease_Entry(struct device unref__ * pDevice)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		      ": OMAPLFBDriverRelease_Entry\n"));
}

static struct platform_driver omaplfb_driver = {
	.driver		= {
		   .name = DRVNAME,
	},
	.suspend	= OMAPLFBDriverSuspend_Entry,
	.resume		= OMAPLFBDriverResume_Entry,
	.shutdown	= OMAPLFBDriverShutdown_Entry,
};

static struct platform_device omaplfb_device = {
	.name		= DEVNAME,
	.id		= -1,
	.dev		= {
		.release = OMAPLFBDeviceRelease_Entry
	}
};

static int __init OMAPLFB_Init(void)
{
	int error;

	if (OMAPLFBInit() != PVRSRV_OK) {
		printk(KERN_WARNING DRIVER_PREFIX
		       ": OMAPLFB_Init: OMAPLFBInit failed\n");
		return -ENODEV;
	}
	error = platform_driver_register(&omaplfb_driver);
	if (error) {
		printk(KERN_WARNING DRIVER_PREFIX
		    ": OMAPLFB_Init: Unable to register platform driver (%d)\n",
		       error);

		goto ExitDeinit;
	}

	error = platform_device_register(&omaplfb_device);
	if (error) {
		printk(KERN_WARNING DRIVER_PREFIX
		   ": OMAPLFB_Init:  Unable to register platform device (%d)\n",
		       error);

		goto ExitDriverUnregister;
	}

	return 0;

ExitDriverUnregister:
	platform_driver_unregister(&omaplfb_driver);

ExitDeinit:
	if (OMAPLFBDeinit() != PVRSRV_OK)
		printk(KERN_WARNING DRIVER_PREFIX
		       ": OMAPLFB_Init: OMAPLFBDeinit failed\n");

	return -ENODEV;
}

static void __exit OMAPLFB_Cleanup(void)
{
	platform_device_unregister(&omaplfb_device);
	platform_driver_unregister(&omaplfb_driver);

	if (OMAPLFBDeinit() != PVRSRV_OK)
		printk(KERN_WARNING DRIVER_PREFIX
		       ": OMAPLFB_Cleanup: OMAPLFBDeinit failed\n");
}

module_init(OMAPLFB_Init);
module_exit(OMAPLFB_Cleanup);
