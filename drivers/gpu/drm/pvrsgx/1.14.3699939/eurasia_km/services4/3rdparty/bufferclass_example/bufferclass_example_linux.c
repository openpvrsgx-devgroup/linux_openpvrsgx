/*************************************************************************/ /*!
@Title          bufferclass example linux specific implementations
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

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the IMG POWERVR
 Services driver with 3rd Party display hardware.  It is NOT a specification for
 a display controller driver, rather a specification to extend the API for a
 pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG POWERVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.
 
 Functions of the API include
 - query primary surface attributes (width, height, stride, pixel format, CPU
     physical and virtual address)
 - swap/flip chain creation and subsequent query of surface attributes
 - asynchronous display surface flipping, taking account of asynchronous read
 (flip) and write (render) operations to the display surface

 Note: having queried surface attributes the client drivers are able to map the
 display memory to any IMG POWERVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.

 This code is intended to be an example of how a pre-existing display driver may
 be extended to support the 3rd Party Display interface to POWERVR Services
 - IMG is not providing a display driver implementation.
 **************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#if defined(LMA)
#include <linux/pci.h>
#else
#include <linux/dma-mapping.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
#include <linux/mutex.h>
#endif

#if defined(BC_DISCONTIG_BUFFERS)
#include <linux/vmalloc.h>
#endif

#include "bufferclass_example.h"
#include "bufferclass_example_linux.h"
#include "bufferclass_example_private.h"

#include "pvrmodule.h"

#define DEVNAME	"bc_example"
#define	DRVNAME	DEVNAME

#if defined(BCE_USE_SET_MEMORY)
#undef BCE_USE_SET_MEMORY
#endif

#if (defined(__i386__) || defined(__x86_64__)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)) && defined(SUPPORT_LINUX_X86_PAT) && defined(SUPPORT_LINUX_X86_WRITECOMBINE)
#include <asm/cacheflush.h>
#define	BCE_USE_SET_MEMORY
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static long BC_Example_Bridge_Unlocked(struct file *file, unsigned int cmd, unsigned long arg);
#else
static int BC_Example_Bridge(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static DEFINE_MUTEX(sBCExampleBridgeMutex);
#endif

#if defined(LDM_PLATFORM) || defined(LDM_PCI)
/*
 * Device class used for /sys entries (and udev device node creation)
 */
static struct class *psPvrClass;
#endif

/*
 * This is the major number we use for all nodes in /dev.
 */
static int AssignedMajorNumber;

static struct file_operations bufferclass_example_fops = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	.unlocked_ioctl = BC_Example_Bridge_Unlocked
#else
	.ioctl = BC_Example_Bridge
#endif
};


#define unref__ __attribute__ ((unused))

#if defined(LMA)
#define PVR_BUFFERCLASS_MEMOFFSET (220 * 1024 * 1024) /* Must be after services localmem region */
#define PVR_BUFFERCLASS_MEMSIZE	  (4 * 1024 * 1024)	  /* Must be before displayclass localmem region */

unsigned long g_ulMemBase = 0;
unsigned long g_ulMemCurrent = 0;

/* PVR device vendor ID */
#define VENDOR_ID_PVR               0x1010
#define DEVICE_ID_PVR               0x1CF1

#define DEVICE_ID1_PVR              0x1CF2


/* PDP mem (including HP mapping) on base register 2 */
#define PVR_MEM_PCI_BASENUM         2
#endif


/*****************************************************************************
 Function Name:	BC_Example_ModInit
 Description  :	Insert the driver into the kernel.

				The device major number is allocated by the kernel dynamically
				if AssignedMajorNumber is zero on entry.  This means that the
				device node (nominally /dev/bc_example) may need to be re-made if
				the kernel varies the major number it assigns.  The number
				does seem to stay constant between runs, but I don't think
				this is guaranteed. The node is made as root on the shell
				with:

						mknod /dev/bc_example c ? 0

				where ? is the major number reported by the printk() - look
				at the boot log using `dmesg' to see this).

				__init places the function in a special memory section that
				the kernel frees once the function has been run.  Refer also
				to module_init() macro call below.

*****************************************************************************/
static int __init BC_Example_ModInit(void)
{
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
    struct device *psDev;
#endif

#if defined(LMA)
	struct pci_dev *psPCIDev;
	int error;
#endif

#if defined(LMA)
	psPCIDev = pci_get_device(VENDOR_ID_PVR, DEVICE_ID_PVR, NULL);
	if (psPCIDev == NULL)
	{
		/* Try an alternative PCI ID */
		psPCIDev = pci_get_device(VENDOR_ID_PVR, DEVICE_ID1_PVR, NULL);
	}

	if (psPCIDev == NULL)
	{
		printk(KERN_ERR DRVNAME ": BC_Example_ModInit:  pci_get_device failed\n");

		goto ExitError;
	}

	if ((error = pci_enable_device(psPCIDev)) != 0)
	{
		printk(KERN_ERR DRVNAME ": BC_Example_ModInit: pci_enable_device failed (%d)\n", error);
		goto ExitError;
	}
#endif

	AssignedMajorNumber = register_chrdev(0, DEVNAME, &bufferclass_example_fops);

	if (AssignedMajorNumber <= 0)
	{
		printk(KERN_ERR DRVNAME ": BC_Example_ModInit: unable to get major number\n");

		goto ExitDisable;
	}

#if defined(DEBUG)
	printk(KERN_ERR DRVNAME ": BC_Example_ModInit: major device %d\n", AssignedMajorNumber);
#endif

#if defined(LDM_PLATFORM) || defined(LDM_PCI)
	/*
	 * This code (using GPL symbols) facilitates automatic device
	 * node creation on platforms with udev (or similar).
	 */
	psPvrClass = class_create(THIS_MODULE, "bc_example");

	if (IS_ERR(psPvrClass))
	{
		printk(KERN_ERR DRVNAME ": BC_Example_ModInit: unable to create class (%ld)", PTR_ERR(psPvrClass));
		goto ExitUnregister;
	}

	psDev = device_create(psPvrClass, NULL, MKDEV(AssignedMajorNumber, 0),
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
						  NULL,
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26)) */
						  DEVNAME);
	if (IS_ERR(psDev))
	{
		printk(KERN_ERR DRVNAME ": BC_Example_ModInit: unable to create device (%ld)", PTR_ERR(psDev));
		goto ExitDestroyClass;
	}
#endif /* defined(LDM_PLATFORM) || defined(LDM_PCI) */

#if defined(LMA)
	/*
	 * We don't do a pci_request_region for  PVR_MEM_PCI_BASENUM, 
	 * we assume the SGX driver has done this already.
	 */
	g_ulMemBase =  pci_resource_start(psPCIDev, PVR_MEM_PCI_BASENUM) + PVR_BUFFERCLASS_MEMOFFSET;
#endif

	if(BC_Example_Init() != BCE_OK)
	{
		printk (KERN_ERR DRVNAME ": BC_Example_ModInit: can't init device\n");
		goto ExitUnregister;
	}

#if defined(LMA)
	/*
	 * To prevent possible problems with system suspend/resume, we don't
	 * keep the device enabled, but rely on the fact that the SGX driver
	 * will have done a pci_enable_device.
	 */
	pci_disable_device(psPCIDev);
#endif

	return 0;

#if defined(LDM_PLATFORM) || defined(LDM_PCI)
ExitDestroyClass:
	class_destroy(psPvrClass);
#endif
ExitUnregister:
	unregister_chrdev(AssignedMajorNumber, DEVNAME);
ExitDisable:
#if defined(LMA)
	pci_disable_device(psPCIDev);
ExitError:
#endif
	return -EBUSY;
} /*BC_Example_ModInit*/

/*****************************************************************************
 Function Name:	BC_Example_ModInit
 Description  :	Remove the driver from the kernel.

				__exit places the function in a special memory section that
				the kernel frees once the function has been run.  Refer also
				to module_exit() macro call below.

*****************************************************************************/
static void __exit BC_Example_ModCleanup(void)
{
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
	device_destroy(psPvrClass, MKDEV(AssignedMajorNumber, 0));
	class_destroy(psPvrClass);
#endif

	unregister_chrdev(AssignedMajorNumber, DEVNAME);
	
	if(BC_Example_Deinit() != BCE_OK)
	{
		printk (KERN_ERR DRVNAME ": BC_Example_ModCleanup: can't deinit device\n");
	}

} /*BC_Example_ModCleanup*/


void *BCAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void BCFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

#if defined(BC_DISCONTIG_BUFFERS)

#define RANGE_TO_PAGES(range) (((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)
#define	VMALLOC_TO_PAGE_PHYS(vAddr) page_to_phys(vmalloc_to_page(vAddr))

BCE_ERROR BCAllocDiscontigMemory(unsigned long ulSize,
                              BCE_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **ppPhysAddr)
{
	unsigned long ulPages = RANGE_TO_PAGES(ulSize);
	IMG_SYS_PHYADDR *pPhysAddr;
	unsigned long ulPage;
	IMG_CPU_VIRTADDR LinAddr;

	LinAddr = __vmalloc(ulSize, GFP_KERNEL | __GFP_HIGHMEM, pgprot_noncached(PAGE_KERNEL));
	if (!LinAddr)
	{
		return BCE_ERROR_OUT_OF_MEMORY;
	}

	pPhysAddr = kmalloc(ulPages * sizeof(IMG_SYS_PHYADDR), GFP_KERNEL);
	if (!pPhysAddr)
	{
		vfree(LinAddr);
		return BCE_ERROR_OUT_OF_MEMORY;
	}

	*pLinAddr = LinAddr;

	for (ulPage = 0; ulPage < ulPages; ulPage++)
	{
		pPhysAddr[ulPage].uiAddr = VMALLOC_TO_PAGE_PHYS(LinAddr);

		LinAddr += PAGE_SIZE;
	}

	*ppPhysAddr = pPhysAddr;

	return BCE_OK;
}

void BCFreeDiscontigMemory(unsigned long ulSize,
                         BCE_HANDLE unref__ hMemHandle,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr)
{
	kfree(pPhysAddr);

	vfree(LinAddr);
}
#else	/* defined(BC_DISCONTIG_BUFFERS) */

BCE_ERROR BCAllocContigMemory(unsigned long ulSize,
                              BCE_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_CPU_PHYADDR *pPhysAddr)
{
#if defined(LMA)
	void *pvLinAddr;
	
	/* Only allowed a certain amount of memory for bufferclass buffers */
	if(g_ulMemCurrent + ulSize >= PVR_BUFFERCLASS_MEMSIZE)
	{
		return (BCE_ERROR_OUT_OF_MEMORY);
	}

	pvLinAddr = ioremap(g_ulMemBase + g_ulMemCurrent, ulSize);

	if(pvLinAddr)
	{
		pPhysAddr->uiAddr = g_ulMemBase + g_ulMemCurrent;
		*pLinAddr = pvLinAddr;	

		/* Not a real allocator; just increment the current address */
		g_ulMemCurrent += ulSize;
		return (BCE_OK);
	}
	return (BCE_ERROR_OUT_OF_MEMORY);
#else	/* defined(LMA) */
#if defined(BCE_USE_SET_MEMORY)
	void *pvLinAddr;
	unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
	int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);
	int iError;

	pvLinAddr = kmalloc(ulAlignedSize, GFP_KERNEL);
	BUG_ON(((unsigned long)pvLinAddr)  & ~PAGE_MASK);

	iError = set_memory_wc((unsigned long)pvLinAddr, iPages);
	if (iError != 0)
	{
		printk(KERN_ERR DRVNAME ": BCAllocContigMemory:  set_memory_wc failed (%d)\n", iError);
		return (BCE_ERROR_OUT_OF_MEMORY);
	}

	pPhysAddr->uiAddr = virt_to_phys(pvLinAddr);
	*pLinAddr = pvLinAddr;

	return (BCE_OK);
#else	/* BCE_USE_SET_MEMORY */
	dma_addr_t dma;
	void *pvLinAddr;

	pvLinAddr = dma_alloc_coherent(NULL, ulSize, &dma, GFP_KERNEL);
	if (pvLinAddr == NULL)
	{
		return (BCE_ERROR_OUT_OF_MEMORY);
	}

	pPhysAddr->uiAddr = dma;
	*pLinAddr = pvLinAddr;

	return (BCE_OK);
#endif	/* BCE_USE_SET_MEMORY */
#endif	/* defined(LMA) */
}

void BCFreeContigMemory(unsigned long ulSize,
                        BCE_HANDLE unref__ hMemHandle,
                        IMG_CPU_VIRTADDR LinAddr,
                        IMG_CPU_PHYADDR PhysAddr)
{
#if defined(LMA)
	g_ulMemCurrent -= ulSize;
	iounmap(LinAddr);
#else	/* defined(LMA) */
#if defined(BCE_USE_SET_MEMORY)
	unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
	int iError;
	int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);

	iError = set_memory_wb((unsigned long)LinAddr, iPages);
	if (iError != 0)
	{
		printk(KERN_ERR DRVNAME ": BCFreeContigMemory:  set_memory_wb failed (%d)\n", iError);
	}
	kfree(LinAddr);
#else	/* BCE_USE_SET_MEMORY */
	dma_free_coherent(NULL, ulSize, LinAddr, (dma_addr_t)PhysAddr.uiAddr);
#endif	/* BCE_USE_SET_MEMORY */
#endif	/* defined(LMA) */
}
#endif	/* defined(BC_DISCONTIG_BUFFERS) */

/**************************************************************************
	FUNCTION:   CpuPAddrToSysPAddrBC
	PURPOSE:    Compute a system physical address from a cpu physical
	            address.
	PARAMETERS:	In:  cpu_paddr - cpu physical address.
	RETURNS:	system physical address.
 **************************************************************************/
IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC(IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;
	
	/* This would only be an inequality if the CPU's MMU did not point to sys address 0, 
	   ie. multi CPU system */
	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}

/**************************************************************************
	FUNCTION:   SysPAddrToCpuPAddrBC
	PURPOSE:    Compute a cpu physical address
	            from a system physical address.
	PARAMETERS:	In:  cpu_paddr - system physical address.
	RETURNS:	cpu physical address.
 **************************************************************************/
IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC(IMG_SYS_PHYADDR sys_paddr)
{
	
	IMG_CPU_PHYADDR cpu_paddr;
	/* This would only be an inequality if the CPU's MMU did not point to sys address 0, 
	   ie. multi CPU system */
	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}

BCE_ERROR BCOpenPVRServices (BCE_HANDLE *phPVRServices)
{
	/* Nothing to do - we have already checked services module insertion */
	*phPVRServices = 0;
	return (BCE_OK);
}


BCE_ERROR BCClosePVRServices (BCE_HANDLE unref__ hPVRServices)
{
	/* Nothing to do */
	return (BCE_OK);
}

BCE_ERROR BCGetLibFuncAddr (BCE_HANDLE unref__ hExtDrv, char *szFunctionName, PFN_BC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetBufferClassJTable", szFunctionName) != 0)
	{
		return (BCE_ERROR_INVALID_PARAMS);
	}

	/* Nothing to do - should be exported from pvrsrv.ko */
	*ppfnFuncTable = PVRGetBufferClassJTable;

	return (BCE_OK);
}


static int BC_Example_Bridge(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = -EFAULT;
	int command = _IOC_NR(cmd);
	BC_Example_ioctl_package sBridge;

	PVR_UNREFERENCED_PARAMETER(inode);

	if (copy_from_user(&sBridge, (void *)arg, sizeof(sBridge)) != 0)
	{
		return err;
	}

	switch(command)
	{
		case _IOC_NR(BC_Example_ioctl_fill_buffer):
		{
			if(FillBuffer(sBridge.inputparam) == -1)
			{
				return err;
			}
			break;
		}
		case _IOC_NR(BC_Example_ioctl_get_buffer_count):
		{
			if(GetBufferCount(&sBridge.outputparam) == -1)
			{
				return err;
			}
			break;
		}
	    case _IOC_NR(BC_Example_ioctl_reconfigure_buffer):
		{
			if(ReconfigureBuffer(&sBridge.outputparam) == -1)
			{
				return err;
			}
			break;
		}
		default:
			return err;
	}

	if (copy_to_user((void *)arg, &sBridge, sizeof(sBridge)) != 0)
	{
		return err;
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static long BC_Example_Bridge_Unlocked(struct file *file, unsigned int cmd, unsigned long arg)
{
	int res;

	mutex_lock(&sBCExampleBridgeMutex);
	res = BC_Example_Bridge(NULL, file, cmd, arg);
	mutex_unlock(&sBCExampleBridgeMutex);

	return res;
}
#endif

/*
 These macro calls define the initialisation and removal functions of the
 driver.  Although they are prefixed `module_', they apply when compiling
 statically as well; in both cases they define the function the kernel will
 run to start/stop the driver.
*/
module_init(BC_Example_ModInit);
module_exit(BC_Example_ModCleanup);

