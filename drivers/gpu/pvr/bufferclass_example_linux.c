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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <linux/dma-mapping.h>

#include "kernelbuffer.h"
#include "bufferclass_example.h"
#include "bufferclass_example_linux.h"
#include "bufferclass_example_private.h"
#include "pvrmodule.h"

#define DEVNAME	"bc_example"
#define	DRVNAME	DEVNAME

MODULE_SUPPORTED_DEVICE(DEVNAME);

static int AssignedMajorNumber;

#define unref__ __attribute__ ((unused))



void *BCAllocKernelMem(u32 ui32Size)
{
	return kmalloc(ui32Size, GFP_KERNEL);
}

void BCFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

enum PVRSRV_ERROR BCAllocContigMemory(u32 ui32Size, void *unref__ * phMemHandle,
				      void __iomem **pLinAddr,
				      struct IMG_CPU_PHYADDR *pPhysAddr)
{
	dma_addr_t dma;
	void *pvLinAddr;

	pvLinAddr = dma_alloc_coherent(NULL, ui32Size, &dma, GFP_KERNEL);

	if (pvLinAddr == NULL)
		return PVRSRV_ERROR_OUT_OF_MEMORY;

	pPhysAddr->uiAddr = dma;
	*pLinAddr = (void __force __iomem *)pvLinAddr;

	return PVRSRV_OK;
}

void BCFreeContigMemory(u32 ui32Size, void *unref__ hMemHandle,
			void __iomem *LinAddr, struct IMG_CPU_PHYADDR PhysAddr)
{
	dma_free_coherent(NULL, ui32Size, (void __force *)LinAddr,
			  (dma_addr_t)PhysAddr.uiAddr);
}

struct IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC(struct IMG_CPU_PHYADDR cpu_paddr)
{
	struct IMG_SYS_PHYADDR sys_paddr;

	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}

struct IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC(struct IMG_SYS_PHYADDR sys_paddr)
{

	struct IMG_CPU_PHYADDR cpu_paddr;

	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}

enum PVRSRV_ERROR BCOpenPVRServices(void **phPVRServices)
{

	*phPVRServices = NULL;
	return PVRSRV_OK;
}

enum PVRSRV_ERROR BCClosePVRServices(void *unref__ hPVRServices)
{

	return PVRSRV_OK;
}

enum PVRSRV_ERROR BCGetLibFuncAddr(void *unref__ hExtDrv, char *szFunctionName,
	  IMG_BOOL (**ppfnFuncTable)(struct PVRSRV_BC_BUFFER2SRV_KMJTABLE *))
{
	if (strcmp("PVRGetBufferClassJTable", szFunctionName) != 0)
		return PVRSRV_ERROR_INVALID_PARAMS;

	*ppfnFuncTable = PVRGetBufferClassJTable;

	return PVRSRV_OK;
}

static int BC_Example_Bridge(struct inode *inode, struct file *file,
			     unsigned int cmd, unsigned long arg)
{
	int err = -EFAULT;
	int command = _IOC_NR(cmd);
	struct BC_Example_ioctl_package __user *psBridge =
			(struct BC_Example_ioctl_package __user *)arg;

	if (!access_ok
	    (VERIFY_WRITE, psBridge, sizeof(struct BC_Example_ioctl_package)))
		return err;

	switch (command) {
	case _IOC_NR(BC_Example_ioctl_fill_buffer):
		if (FillBuffer(psBridge->inputparam) == -1)
			return err;
		break;
	case _IOC_NR(BC_Example_ioctl_get_buffer_count):
		if (GetBufferCount(&psBridge->outputparam) == -1)
			return err;
		break;
	default:
		return err;
	}

	return 0;
}

static const struct file_operations bufferclass_example_fops = {
	.ioctl = BC_Example_Bridge,
};

static int __init BC_Example_ModInit(void)
{
	AssignedMajorNumber =
	    register_chrdev(0, DEVNAME, &bufferclass_example_fops);

	if (AssignedMajorNumber <= 0) {
		printk(KERN_ERR DRVNAME
		       ": BC_Example_ModInit: unable to get major number\n");

		goto ExitDisable;
	}
#if defined(CONFIG_PVR_DEBUG_EXTRA)
	printk(KERN_ERR DRVNAME ": BC_Example_ModInit: major device %d\n",
	       AssignedMajorNumber);
#endif


	if (BC_Example_Init() != PVRSRV_OK) {
		printk(KERN_ERR DRVNAME
		       ": BC_Example_ModInit: can't init device\n");
		goto ExitUnregister;
	}

	return 0;

ExitUnregister:
	unregister_chrdev(AssignedMajorNumber, DEVNAME);
ExitDisable:
	return -EBUSY;
}

static void __exit BC_Example_ModCleanup(void)
{
	unregister_chrdev(AssignedMajorNumber, DEVNAME);

	if (BC_Example_Deinit() != PVRSRV_OK)
		printk(KERN_ERR DRVNAME
		       ": BC_Example_ModCleanup: can't deinit device\n");

}

module_init(BC_Example_ModInit);
module_exit(BC_Example_ModCleanup);
