/*************************************************************************/ /*!
@Title          DRM stub functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

/*
 * Emulate enough of the PCI layer to allow the Linux DRM module to be used
 * with a non-PCI device.
 * Only one device is supported at present.
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,15,0))
#include <asm/system.h>
#endif

#include "pvr_drm_mod.h"

#define	DRV_MSG_PREFIX_STR "pvr drm: "

#define	SGX_VENDOR_ID		1
#define	SGX_DEVICE_ID		1
#define	SGX_SUB_VENDOR_ID	1
#define	SGX_SUB_DEVICE_ID	1

#if defined(DEBUG)
#define	DEBUG_PRINTK(format, args...) printk(format, ## args)
#else
#define	DEBUG_PRINTK(format, args...)
#endif

#define	CLEAR_STRUCT(x) memset(&(x), 0, sizeof(x))

/*
 * Don't specify any initialisers for pvr_pci_bus and pvr_pci_dev, as they
 * will be cleared to zero on unregister.  This has to be done for
 * pvr_pci_dev to prevent a warning from the kernel if the device is
 * re-registered without unloading the DRM module.
 */
static struct pci_bus pvr_pci_bus;
static struct pci_dev pvr_pci_dev;

static bool bDeviceIsRegistered;

static void
release_device(struct device *dev)
{
}

int pci_bus_read_config_dword(struct pci_bus *bus, unsigned int devfn,int where, u32 *val) 
{ 
	return 0;
 
}



int
drm_pvr_dev_add(void)
{
	int ret;

	DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s\n", __FUNCTION__);

	if (bDeviceIsRegistered)
	{
		DEBUG_PRINTK(KERN_WARNING DRV_MSG_PREFIX_STR "%s: Device already registered\n", __FUNCTION__);
		return 0;
	}

	/* Set the device ID */
	pvr_pci_dev.vendor = SGX_VENDOR_ID;
	pvr_pci_dev.device = SGX_DEVICE_ID;
	pvr_pci_dev.subsystem_vendor = SGX_SUB_VENDOR_ID;
	pvr_pci_dev.subsystem_device = SGX_SUB_DEVICE_ID;

	/* drm_set_busid needs the bus number */
	pvr_pci_dev.bus = &pvr_pci_bus;

	dev_set_name(&pvr_pci_dev.dev, "%s", "SGX");
	pvr_pci_dev.dev.release = release_device;

	ret = device_register(&pvr_pci_dev.dev);
	if (ret != 0)
	{
		printk(KERN_ERR DRV_MSG_PREFIX_STR "%s: device_register failed (%d)\n", __FUNCTION__, ret);
	}

	bDeviceIsRegistered = true;

	return ret;
}
EXPORT_SYMBOL(drm_pvr_dev_add);

void
drm_pvr_dev_remove(void)
{
	DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s\n", __FUNCTION__);

	if (bDeviceIsRegistered)
	{
		DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s: Unregistering device\n", __FUNCTION__);

		device_unregister(&pvr_pci_dev.dev);
		bDeviceIsRegistered = false;

		/* Prevent kernel warnings on re-register */
		CLEAR_STRUCT(pvr_pci_dev);
		CLEAR_STRUCT(pvr_pci_bus);
	}
	else
	{
		DEBUG_PRINTK(KERN_WARNING DRV_MSG_PREFIX_STR "%s: Device not registered\n", __FUNCTION__);
	}
}
EXPORT_SYMBOL(drm_pvr_dev_remove);

void
pci_disable_device(struct pci_dev *dev)
{
}

struct pci_dev *
pci_dev_get(struct pci_dev *dev)
{
	return dev;
}

void
pci_set_master(struct pci_dev *dev)
{
}

#define	PCI_ID_COMP(field, value) (((value) == PCI_ANY_ID) || \
			((field) == (value)))

struct pci_dev *
pci_get_subsys(unsigned int vendor, unsigned int device,
	unsigned int ss_vendor, unsigned int ss_device, struct pci_dev *from)
{
	if (from == NULL &&
		PCI_ID_COMP(pvr_pci_dev.vendor, vendor) &&
		PCI_ID_COMP(pvr_pci_dev.device, device) &&
		PCI_ID_COMP(pvr_pci_dev.subsystem_vendor, ss_vendor) &&
		PCI_ID_COMP(pvr_pci_dev.subsystem_device, ss_device))
	{
			DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s: Found %x %x %x %x\n", __FUNCTION__, vendor, device, ss_vendor, ss_device);

			return &pvr_pci_dev;
	}

	if (from == NULL)
	{
		DEBUG_PRINTK(KERN_INFO DRV_MSG_PREFIX_STR "%s: Couldn't find %x %x %x %x\n", __FUNCTION__, vendor, device, ss_vendor, ss_device);
	}

	return NULL;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))
int
pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{
	return 0;
}
#endif

void
pci_unregister_driver(struct pci_driver *drv)
{
}

int
__pci_register_driver(struct pci_driver *drv, struct module *owner,
	const char *mod_name)
{
	return 0;
}

int
pci_enable_device(struct pci_dev *dev)
{
	return 0;
}

void
__bad_cmpxchg(volatile void *ptr, int size)
{
	printk(KERN_ERR DRV_MSG_PREFIX_STR "%s: ptr %p size %u\n",
		__FUNCTION__, ptr, size);
}

