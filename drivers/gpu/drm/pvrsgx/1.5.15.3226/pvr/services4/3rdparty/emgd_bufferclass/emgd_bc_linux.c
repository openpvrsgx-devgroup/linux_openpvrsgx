/****************************************************************************
 *
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************/

#define MODULE_NAME hal.buf_class

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#if 0
#if defined(LMA)
#include <linux/pci.h>
#else
#include <linux/dma-mapping.h>
#endif
#endif
#include <drm/drmP.h>
#include <drm/drm.h>
#include "emgd_drv.h"
#include "io.h"
#include "pvrmodule.h"
#include "emgd_drm.h"
#include "igd_gmm.h"
#include "emgd_bc.h"
#if 0
#define DEVNAME    "bc_video"
#define DRVNAME    DEVNAME
#endif
#define BC_FOURCC(a,b,c,d) \
    ((unsigned long) ((a) | (b)<<8 | (c)<<16 | (d)<<24))

#define BC_PIX_FMT_NV12     BC_FOURCC('N', 'V', '1', '2')    /*YUV 4:2:0 */
#define BC_PIX_FMT_UYVY     BC_FOURCC('U', 'Y', 'V', 'Y')    /*YUV 4:2:2 */
#define BC_PIX_FMT_YUYV     BC_FOURCC('Y', 'U', 'Y', 'V')    /*YUV 4:2:2 */
#define BC_PIX_FMT_RGB565   BC_FOURCC('R', 'G', 'B', 'P')    /*RGB 5:6:5 */
#define BC_PIX_FMT_ARGB   BC_FOURCC('A','R', 'G', 'B')    /*ARGB*/
#define BC_PIX_FMT_xRGB   BC_FOURCC('x','R', 'G', 'B')    /*xRGB*/
#define BC_PIX_FMT_RGB4	BC_FOURCC('R', 'G', 'B', '4') /*RGB4*/
#if 0
#if defined(BCE_USE_SET_MEMORY)
#undef BCE_USE_SET_MEMORY
#endif

#if defined(__i386__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)) && defined(SUPPORT_LINUX_X86_PAT) && defined(SUPPORT_LINUX_X86_WRITECOMBINE)
#include <asm/cacheflush.h>
#define    BCE_USE_SET_MEMORY
#endif

static int width_align;
#endif
extern unsigned int bc_video_id[BUFCLASS_DEVICE_MAX_ID];
extern unsigned int bc_video_id_usage[BUFCLASS_DEVICE_MAX_ID];
extern void* gp_bc_Anchor[BUFCLASS_DEVICE_MAX_ID];

/* Stream Tag. To be stored on Buffer class name array. */
#define MAX_STREAM_TAG 0xFFFFFFF0
unsigned long bc_stream_tag = 0;

typedef enum _BC_DEVICE_STATE {
	BC_DEV_READY = 0xF0,
	BC_DEV_NOT_READY,
} BC_Dev_Status;

#if 0
MODULE_SUPPORTED_DEVICE (DEVNAME);

int FillBuffer(unsigned int uiBufferIndex);

#if defined(LDM_PLATFORM) || defined(LDM_PCI)
static struct class *psPvrClass;
#endif

static int AssignedMajorNumber;

#define unref__ __attribute__ ((unused))

#if defined(LMA)
#define PVR_BUFFERCLASS_MEMOFFSET (220 * 1024 * 1024)
#define PVR_BUFFERCLASS_MEMSIZE      (4 * 1024 * 1024)

unsigned long g_ulMemBase = 0;
unsigned long g_ulMemCurrent = 0;

#define VENDOR_ID_PVR               0x1010
#define DEVICE_ID_PVR               0x1CF1

#define PVR_MEM_PCI_BASENUM         2
#endif

#define file_to_id(file)  (iminor(file->f_path.dentry->d_inode))
#endif
/* FCB #17711*/
/* flag indicates whether BC_Video Module Initialized or not 
* 0 - Uninitialized, 1 - Initialized.
*/
static int flg_bc_ts_init = 0;

/*
** Function: Set device state
** state: 0 - enable; 1 - disable
*/
void emgd_bc_ts_set_state(BC_DEVINFO *psDevInfo, const IMG_CHAR state);

int emgd_bc_ts_init(void){
    int i, j;
#if 0    
    /*LDM_PCI is defined, while LDM_PLATFORM and LMA are not defined.*/
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
    struct device *psDev;
#endif

#if defined(LMA)
    struct pci_dev *psPCIDev;
    int error;
#endif
#endif
	EMGD_TRACE_ENTER;

	if (0 != flg_bc_ts_init) {
		EMGD_ERROR("BC already been initialized!");
		return 0;
	}
#if 0	
    /* video width is 4 byte aligned */
    width_align = 4;
#endif
	memset(bc_video_id, 0, sizeof(bc_video_id));
	memset(bc_video_id_usage, 0, sizeof(bc_video_id_usage));
	memset(gp_bc_Anchor, 0, sizeof(gp_bc_Anchor));
#if 0	
#if defined(LMA)
    psPCIDev = pci_get_device (VENDOR_ID_PVR, DEVICE_ID_PVR, NULL);
    if (psPCIDev == NULL){
        EMGD_ERROR("pci_get_device failed");
        goto ExitError;
    }

    if ((error = pci_enable_device (psPCIDev)) != 0){
        EMGD_ERROR("pci_enable_device failed (%d)", error);
        goto ExitError;
    }
#endif
#endif
#if 0
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
    psPvrClass = class_create(THIS_MODULE, "bc_video");
    if (IS_ERR (psPvrClass)) {
        EMGD_ERROR("unable to create class (%ld)",
                PTR_ERR(psPvrClass));
        goto ExitUnregister;
    }

    psDev = device_create (psPvrClass, NULL, MKDEV (AssignedMajorNumber, 0),
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
            NULL,
#endif
            DEVNAME);
    if (IS_ERR (psDev)){
        EMGD_ERROR("unable to create device (%ld)",
                PTR_ERR (psDev));
        goto ExitDestroyClass;
    }
#endif

#if defined(LMA)
    g_ulMemBase =
        pci_resource_start (psPCIDev,
                PVR_MEM_PCI_BASENUM) + PVR_BUFFERCLASS_MEMOFFSET;
#endif
#endif
    for (i = 0; i < BUFCLASS_DEVICE_MAX_ID; i++) {
        bc_video_id[i] = i;
        bc_video_id_usage[i] = 0;
        if (bc_ts_init(bc_video_id[i]) != EMGD_OK) {
            EMGD_ERROR("can't init video bc device %d.", i);
            for (j = i; j >= 0; j--) {
                bc_ts_uninit(bc_video_id[j]);
            }
            goto ExitUnregister;
        }
    }
#if 0
#if defined(LMA)
    pci_disable_device (psPCIDev);
#endif
#endif
	flg_bc_ts_init = 1;

	EMGD_TRACE_EXIT;

	return 0;
#if 0
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
ExitDestroyClass:
    class_destroy (psPvrClass);
#endif
#endif
ExitUnregister:
#if 0
    unregister_chrdev (AssignedMajorNumber, DEVNAME);
    //ExitDisable:
#if defined(LMA)
    pci_disable_device (psPCIDev);
ExitError:
#endif
#endif
    return -EBUSY;
}

int emgd_bc_ts_uninit(void){
    int i = 0;
#if 0    
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
    device_destroy (psPvrClass, MKDEV (AssignedMajorNumber, 0));
    class_destroy (psPvrClass);
#endif
#endif
	EMGD_TRACE_ENTER;
		
	if (1 != flg_bc_ts_init) {
		EMGD_ERROR("BC not been initialized yet");
		return 0;
	}
	
    for (i = 0; i < BUFCLASS_DEVICE_MAX_ID; i++){
    	if (1 == bc_video_id_usage[i]) {
        	if (bc_ts_uninit(bc_video_id[i]) != EMGD_OK) {
            	EMGD_ERROR("can't deinit video device %d.", i);
            	return -1;
        	}
    	}
    }

	flg_bc_ts_init = 0;
	
	EMGD_TRACE_EXIT;

    return 0;
}
#if 0
#define	RANGE_TO_PAGES(range) (((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)
#define	VMALLOC_TO_PAGE_PHYS(vAddr) page_to_phys(vmalloc_to_page(vAddr))

BCE_ERROR BCAllocDiscontigMemory (unsigned long ulSize,
        BCE_HANDLE unref__ *phMemHandle,
        IMG_CPU_VIRTADDR *pLinAddr,
        IMG_SYS_PHYADDR **ppPhysAddr){
    unsigned long ulPages = RANGE_TO_PAGES (ulSize);
    IMG_SYS_PHYADDR *pPhysAddr = IMG_NULL;
    unsigned long ulPage = 0;
    IMG_CPU_VIRTADDR LinAddr = IMG_NULL;

		EMGD_TRACE_ENTER;

    LinAddr =
        __vmalloc (ulSize, GFP_KERNEL | __GFP_HIGHMEM,
                pgprot_noncached (PAGE_KERNEL));
    if (!LinAddr){
        return BCE_ERROR_OUT_OF_MEMORY;
    }

    pPhysAddr = vmalloc (ulPages * sizeof (IMG_SYS_PHYADDR));
    if (!pPhysAddr) {
        vfree(LinAddr);
        return BCE_ERROR_OUT_OF_MEMORY;
    }

    *pLinAddr = LinAddr;

   for (ulPage = 0; ulPage < ulPages; ulPage++){
        pPhysAddr[ulPage].uiAddr = VMALLOC_TO_PAGE_PHYS (LinAddr);

        LinAddr += PAGE_SIZE;
   }

  *ppPhysAddr = pPhysAddr;
	EMGD_TRACE_EXIT;

  return BCE_OK;
}

void BCFreeDiscontigMemory (unsigned long ulSize,
        BCE_HANDLE unref__ hMemHandle,
        IMG_CPU_VIRTADDR LinAddr, IMG_SYS_PHYADDR * pPhysAddr){

	BCFreeKernelMem(pPhysAddr);
	
    if (NULL != LinAddr) {
    	vfree(LinAddr);
    }
}

BCE_ERROR BCAllocContigMemory (unsigned long ulSize,
        BCE_HANDLE unref__ *phMemHandle,
        IMG_CPU_VIRTADDR *pLinAddr, IMG_CPU_PHYADDR *pPhysAddr){
#if defined(LMA)
    void *pvLinAddr = NULL;

    if (g_ulMemCurrent + ulSize >= PVR_BUFFERCLASS_MEMSIZE){
        return (BCE_ERROR_OUT_OF_MEMORY);
    }

    pvLinAddr = ioremap(g_ulMemBase + g_ulMemCurrent, ulSize);

    if (pvLinAddr)
    {
        pPhysAddr->uiAddr = g_ulMemBase + g_ulMemCurrent;
        *pLinAddr = pvLinAddr;

        g_ulMemCurrent += ulSize;
        return (BCE_OK);
    }
    return (BCE_ERROR_OUT_OF_MEMORY);
#else
#if defined(BCE_USE_SET_MEMORY)
    void *pvLinAddr = NULL;
    unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
    int iPages = (int) (ulAlignedSize >> PAGE_SHIFT);
    int iError;

    pvLinAddr = kmalloc(ulAlignedSize, GFP_KERNEL);
    if(!pvLinAddr) {
        return (BCE_ERROR_OUT_OF_MEMORY);
    }
    BUG_ON (((unsigned long) pvLinAddr) & ~PAGE_MASK);

    iError = set_memory_wc((unsigned long) pvLinAddr, iPages);
    if (iError != 0){
        EMGD_ERROR("set_memory_wc failed (%d)", iError);
        return (BCE_ERROR_OUT_OF_MEMORY);
    }

    pPhysAddr->uiAddr = virt_to_phys (pvLinAddr);
    *pLinAddr = pvLinAddr;

    return (BCE_OK);
#else
    dma_addr_t dma;
    void *pvLinAddr = NULL;

    pvLinAddr = dma_alloc_coherent (NULL, ulSize, &dma, GFP_KERNEL);
    if (NULL == pvLinAddr){
        return (BCE_ERROR_OUT_OF_MEMORY);
    }

    pPhysAddr->uiAddr = dma;
    *pLinAddr = pvLinAddr;

    return (BCE_OK);
#endif
#endif
}

void BCFreeContigMemory (unsigned long ulSize,
        BCE_HANDLE unref__ hMemHandle,
        IMG_CPU_VIRTADDR LinAddr, IMG_CPU_PHYADDR PhysAddr){
#if defined(LMA)
    g_ulMemCurrent -= ulSize;
    iounmap(LinAddr);
#else
#if defined(BCE_USE_SET_MEMORY)
    unsigned long ulAlignedSize = PAGE_ALIGN (ulSize);
    int iError;
    int iPages = (int) (ulAlignedSize >> PAGE_SHIFT);

    iError = set_memory_wb ((unsigned long) LinAddr, iPages);
    if (iError != 0) {
        EMGD_ERROR("set_memory_wb failed (%d)", iError);
    }
    BCFreeKernelMem(LinAddr);
#else
    dma_free_coherent (NULL, ulSize, LinAddr, (dma_addr_t)PhysAddr.uiAddr);
#endif
#endif
}

IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC (IMG_CPU_PHYADDR cpu_paddr){
    IMG_SYS_PHYADDR sys_paddr;
    sys_paddr.uiAddr = cpu_paddr.uiAddr;
    return sys_paddr;
}

IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC (IMG_SYS_PHYADDR sys_paddr){
    IMG_CPU_PHYADDR cpu_paddr;
    cpu_paddr.uiAddr = sys_paddr.uiAddr;
    return cpu_paddr;
}
#endif
emgd_error_t BCOpenPVRServices (BCE_HANDLE * phPVRServices){
    *phPVRServices = 0;
    return (EMGD_OK);
}


emgd_error_t BCClosePVRServices (BCE_HANDLE unref__ hPVRServices){
    return (EMGD_OK);
}

emgd_error_t BCGetLibFuncAddr (BCE_HANDLE unref__ hExtDrv, char *szFunctionName,
        PFN_BC_GET_PVRJTABLE * ppfnFuncTable) {
    if (strcmp ("PVRGetBufferClassJTable", szFunctionName) != 0){
        return (EMGD_ERROR_INVALID_PARAMS);
    }

    *ppfnFuncTable = PVRGetBufferClassJTable;

    return (EMGD_OK);
}

int BC_CreateBuffers(BC_DEVINFO *psDevInfo, bc_buf_params_t *p, IMG_BOOL is_conti_addr) {
    IMG_UINT32 i = 0, j = 0;
	IMG_UINT32 stride = 0;
	IMG_UINT32 size = 0;
    PVRSRV_PIXEL_FORMAT pixel_fmt = 0;
	BC_BUFFER *bufnode = NULL;
	
	EMGD_TRACE_ENTER;

	if (p->count < 1) {
  		return -1;
  	}    

	if (p->count > BUFCLASS_BUFFER_MAX) {
  		return -1;
  	}

	if (p->width <= 1 || p->height <= 1 || p->stride <=1 ) {
        return -1;
    }

	if (p->width > 2048 || p->height > 1536 || p->stride >2048*4) {
        return -1;
    }

    switch (p->fourcc) {
        case BC_PIX_FMT_NV12:
            pixel_fmt = PVRSRV_PIXEL_FORMAT_NV12;
            break;
        case BC_PIX_FMT_UYVY:
            pixel_fmt = PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY;
            break;
        case BC_PIX_FMT_RGB565:
            pixel_fmt = PVRSRV_PIXEL_FORMAT_RGB565;
            p->stride = p->stride << 1;    /* stride for RGB from user space is uncorrect */
            break;
        case BC_PIX_FMT_YUYV:
            pixel_fmt = PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV;
            break;
	case BC_PIX_FMT_ARGB:
	case BC_PIX_FMT_RGB4:
            pixel_fmt = PVRSRV_PIXEL_FORMAT_ARGB8888;
            break;
 	case BC_PIX_FMT_xRGB:
            pixel_fmt = PVRSRV_PIXEL_FORMAT_XRGB8888;
            break;
        default:
            return -1;
            break;
    }

    stride = p->stride;

    if (p->type != BC_MEMORY_MMAP && p->type != BC_MEMORY_USERPTR) {
        return -1;
    }      

	if (0 == psDevInfo->sBufferInfo.ui32BufferCount && NULL == psDevInfo->psSystemBuffer) {
		psDevInfo->psSystemBuffer = (BC_BUFFER *)BCAllocKernelMem(sizeof(BC_BUFFER) * BUFCLASS_BUFFER_MAX);
		if (NULL != psDevInfo->psSystemBuffer) {
			memset((void *)psDevInfo->psSystemBuffer, 0, sizeof(BC_BUFFER) * BUFCLASS_BUFFER_MAX);
		} else {
			return -1;
		}
	} else {
		
		if ((psDevInfo->sBufferInfo.ui32BufferCount + p->count) > BUFCLASS_BUFFER_MAX) {
			EMGD_ERROR("No avaiable Buffers");
			return -1;
		}
	}

    if (0 != psDevInfo->sBufferInfo.ui32BufferCount) {
    	if (psDevInfo->sBufferInfo.ui32Width != p->width
    		|| psDevInfo->sBufferInfo.ui32Height != p->height
    		|| psDevInfo->sBufferInfo.ui32ByteStride != p->stride
    		|| psDevInfo->sBufferInfo.pixelformat != pixel_fmt 
    		|| psDevInfo->buf_type != p->type) {
    		
			EMGD_ERROR("Request invalid buffers");
			return -ENODEV;
    	}
    }    

    psDevInfo->buf_type = p->type;

    size = p->height * stride;
    
    if (pixel_fmt == PVRSRV_PIXEL_FORMAT_NV12) {
        size += (stride >> 1) * (p->height >> 1) << 1;
    }
    
	/* Append new nodes*/
	for (i = 0; i < p->count; i++) {
		bufnode = &(psDevInfo->psSystemBuffer[psDevInfo->sBufferInfo.ui32BufferCount + i]);
		EMGD_DEBUG("Buffer idx - %lu", psDevInfo->sBufferInfo.ui32BufferCount + i);
		bufnode->ulSize = size;
		EMGD_DEBUG("Buffer size - %lu", size);
		
		bufnode->tag = psDevInfo->sBufferInfo.ui32BufferCount + i;

		bufnode->psSyncData = NULL;

		bufnode->is_conti_addr = is_conti_addr;
		EMGD_DEBUG("Buffer IsCont. - %d", is_conti_addr);

		bufnode->psNext = NULL;
		if (is_conti_addr){
        	bufnode->psSysAddr = (IMG_SYS_PHYADDR *)BCAllocKernelMem (sizeof(IMG_SYS_PHYADDR));
            if (NULL == bufnode->psSysAddr) {
				/* Free the previously successfully allocated memeory */
				for(j=0;j<i;j++) {
					bufnode = 
						&(psDevInfo->psSystemBuffer[psDevInfo->sBufferInfo.ui32BufferCount + j]);
					bc_ts_free_bcbuffer(bufnode);	
				}
				return EMGD_ERROR_OUT_OF_MEMORY;
			}
            memset(bufnode->psSysAddr, 0, sizeof(IMG_SYS_PHYADDR));
        } else {
            return EMGD_ERROR_INVALID_PARAMS;
        }
	} 

	if (0 == psDevInfo->sBufferInfo.ui32BufferCount) {
    	psDevInfo->sBufferInfo.pixelformat = pixel_fmt;
    	psDevInfo->sBufferInfo.ui32Width = p->width;
    	psDevInfo->sBufferInfo.ui32Height = p->height;
    	psDevInfo->sBufferInfo.ui32ByteStride = stride;
    	psDevInfo->sBufferInfo.ui32Flags = PVRSRV_BC_FLAGS_YUVCSC_FULL_RANGE |
        	PVRSRV_BC_FLAGS_YUVCSC_BT601;
	}

	psDevInfo->sBufferInfo.ui32BufferCount += p->count;

	EMGD_TRACE_EXIT;
	
    return 0;
}

int BC_DestroyBuffers(void *psDevInfo) {
    BC_DEVINFO *DevInfo = NULL;
    IMG_UINT32 i = 0;
    BC_BUFFER *bufnode = NULL;

   	EMGD_TRACE_ENTER;

    if (NULL == psDevInfo) {
        return -1;
    }    

	DevInfo = (BC_DEVINFO *)psDevInfo;
    EMGD_DEBUG("To Free %lu buffers", DevInfo->sBufferInfo.ui32BufferCount);

	if (DevInfo->sBufferInfo.ui32BufferCount > BUFCLASS_BUFFER_MAX) {
		EMGD_ERROR("Wrong number of buffers");
	}

	for (i = 0; i < DevInfo->sBufferInfo.ui32BufferCount; i++) {
		if(i < BUFCLASS_BUFFER_MAX) {
			bufnode = &(DevInfo->psSystemBuffer[i]);
			bc_ts_free_bcbuffer(bufnode);
		}
	}
	BCFreeKernelMem(DevInfo->psSystemBuffer);
	DevInfo->psSystemBuffer = NULL;

	if (0 != DevInfo->ulRefCount) {
		DevInfo->ulRefCount--;
	}
    DevInfo->sBufferInfo.pixelformat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
    DevInfo->sBufferInfo.ui32Width = 0;
    DevInfo->sBufferInfo.ui32Height = 0;
    DevInfo->sBufferInfo.ui32ByteStride = 0;
    DevInfo->sBufferInfo.ui32Flags = 0;
    DevInfo->sBufferInfo.ui32BufferCount = 0;

	EMGD_TRACE_EXIT;

    return 0;
}

/*
** For Buffer Class of Texture Stream 
*/
static __inline int emgd_bc_ts_bridge_init(struct drm_device *drv, void* arg, struct drm_file *file_priv){
	int err = -EFAULT;
	int i;
	emgd_drm_bc_ts_t *psBridge = (emgd_drm_bc_ts_t *) arg;
	BC_DEVINFO *psDevInfo = NULL;

	EMGD_TRACE_ENTER;

	if (NULL == psBridge) {
		EMGD_ERROR("Invalid input parameter!\n");
		return err;
	}
	psBridge->rtn = 1;
	
	/* search available device */
	for (i = 0; i < BUFCLASS_DEVICE_MAX_ID; i++) {
		if (0 == bc_video_id_usage[i]) {
   			bc_video_id_usage[i] = 1;
			psDevInfo = (BC_DEVINFO *)GetAnchorPtr(bc_video_id[i]);
			if (NULL == psDevInfo) {
				EMGD_ERROR("System Error");
				return err;
			}
			psBridge->dev_id = psDevInfo->sBufferInfo.ui32BufferDeviceID;
			if (MAX_STREAM_TAG == bc_stream_tag) {
				bc_stream_tag = 0;
			} else {	
				bc_stream_tag++;
			}	

			*(IMG_UINT32 *)(psDevInfo->sBufferInfo.szDeviceName + TSBUFFERCLASS_VIDEOID_OFFSET) = bc_stream_tag;

			/* Disable device*/
			emgd_bc_ts_set_state(psDevInfo, 0);
			
			EMGD_DEBUG("Grab a Device - 0x%lx , ID %lu, idx - %d\n", 
				(unsigned long)psDevInfo,
				psBridge->dev_id, 
				bc_video_id[i]);
			psBridge->rtn = 0;
          	err = 0;
      		break;
    	}
 	}
   	  
 	if (BUFCLASS_DEVICE_MAX_ID == i) { 
    	EMGD_ERROR("Do you really need to run more than 6 video simulateously.");
 	}
 	
	EMGD_TRACE_EXIT;

	return err;
}

static __inline int emgd_bc_ts_bridge_uninit(struct drm_device *drv, void* arg, struct drm_file *file_priv)
{
	int err = -EFAULT;
	int i;
	emgd_drm_bc_ts_t *psBridge = (emgd_drm_bc_ts_t *) arg;
	BC_DEVINFO *psDevInfo = NULL; 

	EMGD_TRACE_ENTER;

	if (NULL == psBridge) {
		EMGD_ERROR("Invalid input parameter!\n");
		return err;
	}

	psDevInfo = (BC_DEVINFO *)GetAnchorPtr(psBridge->dev_id);
	if (NULL == psDevInfo) {
		EMGD_ERROR("System Error");
		return err;
	}

	/* To disable buffer class device*/
	emgd_bc_ts_set_state(psDevInfo, 0);	

	if (EMGD_OK == BC_DestroyBuffers((void *)psDevInfo)) {
		EMGD_DEBUG("Free Device -  %lu", psBridge->dev_id);
		bc_video_id_usage[i] = 0,
		psBridge->rtn = 0;
		err = 0;
	} else {
		EMGD_ERROR("Uninit device with id %lu failure!\n", psBridge->dev_id);
	}				
	
	EMGD_TRACE_EXIT;

	return err;
}

static __inline int emgd_bc_ts_bridge_request_buffers(struct drm_device *drv, void* arg, struct drm_file *file_priv)
{
	int err = -EFAULT;
	emgd_drm_bc_ts_t *psBridge = (emgd_drm_bc_ts_t *) arg;
	bc_buf_params_t p;
	BC_DEVINFO *bc_devinfo = NULL;
	IMG_BOOL is_continous = IMG_FALSE;
	
	EMGD_TRACE_ENTER;
	
	if (NULL == psBridge) {
		EMGD_ERROR("Invalid input parameter!\n");
		return err;
	}

	is_continous = psBridge->is_continous ? IMG_TRUE : IMG_FALSE;

	bc_devinfo = (BC_DEVINFO *)GetAnchorPtr(psBridge->dev_id);
	if (NULL == bc_devinfo 
		|| (NULL != bc_devinfo && psBridge->dev_id != bc_devinfo->sBufferInfo.ui32BufferDeviceID)) {
		EMGD_ERROR("input device id is invalid");
		return err;
	}
	if (NULL != bc_devinfo && 1 == bc_video_id_usage[bc_devinfo->sBufferInfo.ui32BufferDeviceID]) {
   		psBridge->buf_id = bc_devinfo->sBufferInfo.ui32BufferCount;
   		p.width = psBridge->width;
   		p.height = psBridge->height;
   		p.count = psBridge->num_buf;
   		p.fourcc = psBridge->pixel_format;
   		p.stride = psBridge->stride;
   		p.type = BC_MEMORY_MMAP;
   		if (0 == BC_CreateBuffers(bc_devinfo, &p, is_continous)) {
			psBridge->rtn = 0;
			err = 0;
       	} else {
			EMGD_ERROR("Request Buffers failure!\n");
       	}
 	}

	EMGD_TRACE_EXIT;

	return err;
}

static __inline int emgd_bc_ts_bridge_release_buffers(struct drm_device *drv, void* arg, struct drm_file *file_priv)
{
	int err = -EFAULT;
	emgd_drm_bc_ts_t *psBridge = (emgd_drm_bc_ts_t *) arg;
	BC_DEVINFO *psDevInfo = NULL;
	
	EMGD_TRACE_ENTER;

	if (NULL == psBridge) {
		EMGD_ERROR("Invalid input parameter!\n");
		return err;
	}

	psDevInfo = (BC_DEVINFO *)GetAnchorPtr(psBridge->dev_id);

	if (NULL == psDevInfo 
		|| (NULL != psDevInfo && psBridge->dev_id != psDevInfo->sBufferInfo.ui32BufferDeviceID)) {
		EMGD_ERROR("input device id is invalid");
		return err;
	}
	
	if (NULL != psDevInfo && 1 == bc_video_id_usage[psDevInfo->sBufferInfo.ui32BufferDeviceID]) {
		emgd_bc_ts_set_state(psDevInfo, 0);
		
   		if (EMGD_OK == BC_DestroyBuffers((void *)psDevInfo)) {
			psBridge->rtn = 0;
			err = EMGD_OK;
       	} else {
			EMGD_ERROR("Release Buffers failure!\n");
		}
	}
	EMGD_TRACE_EXIT;

	return EMGD_OK; 
}

static __inline int emgd_bc_ts_bridge_set_buffer_info(struct drm_device *drv, void* arg, struct drm_file *file_priv)
{
	int err = -EFAULT;
	emgd_drm_bc_ts_t *psBridge = (emgd_drm_bc_ts_t *) arg;
	BC_DEVINFO *devinfo = IMG_NULL;	
	BC_BUFFER *bcBuf = NULL;
	unsigned long virt=0;
	EMGD_TRACE_ENTER;
	
	if (NULL == psBridge) {
		EMGD_ERROR("Invalid input parameter!");
		return err;
	}
	psBridge->rtn = 1;

	EMGD_DEBUG("dev_id %lu, buf_id %lu, buf_tag %lu, phyaddr 0x%lx, virtadd 0x%lx\n", 
			psBridge->dev_id,
			psBridge->buf_id,
			psBridge->buf_tag,
			psBridge->phyaddr,
			psBridge->virtaddr);

	devinfo = (BC_DEVINFO *)GetAnchorPtr(psBridge->dev_id);

	if (NULL == devinfo 
		|| (NULL != devinfo && psBridge->dev_id != devinfo->sBufferInfo.ui32BufferDeviceID)) {
		EMGD_ERROR("input device id is invalid");
		return err;
	}		
	/*  To Set Buffer Class Device State */
 	if (0xFF == psBridge->buf_id && BUFCLASS_BUFFER_MAX < psBridge->buf_id) {
			/* 0xFF - Indicates Buffer Class Device is ready. 0xF0 - not ready */
		if (BC_DEV_READY == psBridge->buf_tag && 0 < devinfo->sBufferInfo.ui32BufferCount) {
			emgd_bc_ts_set_state(devinfo, 1);	
			EMGD_DEBUG("dev_id %lu Enable to be Ready!\n", psBridge->dev_id); 
			goto SUCCESS_OK;
		}
		if (BC_DEV_NOT_READY == psBridge->buf_tag) {
			emgd_bc_ts_set_state(devinfo, 0);
			EMGD_DEBUG("dev_id %lu Disabled!\n", psBridge->dev_id); 
			goto SUCCESS_OK;
		}	
 	}

   	if (psBridge->buf_id >= devinfo->sBufferInfo.ui32BufferCount) {
        EMGD_ERROR("Invalid buf_id");
        return err;
    }

    bcBuf = &(devinfo->psSystemBuffer[psBridge->buf_id]);

	if (NULL == bcBuf) {
        EMGD_ERROR("Invalid buffer: buf_id - %lu!", psBridge->buf_id);
        return err;					
	}
	bcBuf->tag = psBridge->buf_tag;
	
	bcBuf->sCPUVAddr = (IMG_CPU_VIRTADDR)psBridge->virtaddr;
	
	if (IMG_FALSE == bcBuf->is_conti_addr) {
		EMGD_ERROR("Only support conti. memory!");
	} else {
		if(!psBridge->mapped){	

			if(!psBridge->virtaddr) {
		        EMGD_ERROR("Invalid bridge address");
		        return err;
			}
			/*The CPU device will be registerd into PVR later*/
			virt=((struct emgd_ci_meminfo_t *)(psBridge->virtaddr))->virt;
			if(!virt) {
		        EMGD_ERROR("Invalid CPU address");	
				return err;		
			}
			bcBuf->sCPUVAddr = virt;
			bcBuf->psSysAddr[0].uiAddr = virt_to_phys(virt);
			
		}else{
			bcBuf->psSysAddr[0].uiAddr = psBridge->phyaddr;
		}
	}
	bcBuf->mapped = psBridge->mapped;
SUCCESS_OK:    
	psBridge->rtn = 0;
	
	EMGD_TRACE_EXIT;

	return 0;
}
static __inline int emgd_bc_ts_bridge_get_buffers_count(struct drm_device *drv, void* arg, struct drm_file *file_priv)
{
	int err = -EFAULT;
	emgd_drm_bc_ts_t *psBridge = (emgd_drm_bc_ts_t *)arg;
	BC_DEVINFO *psDevInfo = NULL;
	
	EMGD_TRACE_ENTER;

	if (NULL == psBridge) {
		EMGD_ERROR("Invalid input parameter!\n");
		return err;
	}

	psBridge->rtn = 1;

	psDevInfo = (BC_DEVINFO *)GetAnchorPtr(psBridge->dev_id);

	if (NULL == psDevInfo 
		|| (NULL != psDevInfo && psBridge->dev_id != psDevInfo->sBufferInfo.ui32BufferDeviceID)) {
		EMGD_ERROR("input device id is invalid");
		return err;
	}
	
	if (NULL != psDevInfo) {
		psBridge->num_buf = psDevInfo->sBufferInfo.ui32BufferCount;
		psBridge->rtn = 0;
		err = 0;
	}
	EMGD_TRACE_EXIT;

	return err;
}

static __inline int emgd_bc_ts_bridge_get_buffer_index(struct drm_device *drv, void* arg, struct drm_file *file_priv)
{
  	EMGD_DEBUG("Not supported\n");
	return 0;
}

/* For Buffer Class of Texture Stream */
int emgd_bc_ts_cmd_init(struct drm_device *dev, void *arg, struct drm_file *file_priv)
{
	emgd_drm_bc_ts_t *drm_data = (emgd_drm_bc_ts_t *)arg;

	EMGD_TRACE_ENTER;
	
	if (NULL == dev || NULL == arg || NULL == file_priv) {
		return 0;
	}

	drm_data->rtn = emgd_bc_ts_bridge_init(dev, drm_data, file_priv);

	EMGD_DEBUG("drm_data->rtn = %d", drm_data->rtn);

	EMGD_TRACE_EXIT;
	
	return 0;
}

int emgd_bc_ts_cmd_uninit(struct drm_device *dev, void *arg, struct drm_file *file_priv)
{
	emgd_drm_bc_ts_t *drm_data = (emgd_drm_bc_ts_t *)arg;

	EMGD_TRACE_ENTER;
	
	if (NULL == dev || NULL == arg || NULL == file_priv) {
		return 0;
	}

	drm_data->rtn = emgd_bc_ts_bridge_uninit(dev, drm_data, file_priv);

	EMGD_DEBUG("drm_data->rtn = %d", drm_data->rtn);
	EMGD_DEBUG("Returning 0");
	EMGD_TRACE_EXIT;
	
	return 0;
}
int emgd_bc_ts_cmd_request_buffers(struct drm_device *dev, void *arg, struct drm_file *file_priv)
{
	emgd_drm_bc_ts_t *drm_data = (emgd_drm_bc_ts_t *)arg;

	EMGD_TRACE_ENTER;
	
	if (NULL == dev || NULL == arg || NULL == file_priv) {
		return 0;
	}

	drm_data->rtn = emgd_bc_ts_bridge_request_buffers(dev, drm_data, file_priv);

	EMGD_DEBUG("drm_data->rtn = %d", drm_data->rtn);
	EMGD_DEBUG("Returning 0");
	EMGD_TRACE_EXIT;
	
	return 0;

}
int emgd_bc_ts_cmd_release_buffers(struct drm_device *dev, void *arg, struct drm_file *file_priv)
{
	emgd_drm_bc_ts_t *drm_data = (emgd_drm_bc_ts_t *)arg;

	EMGD_TRACE_ENTER;
	
	if (NULL == dev || NULL == arg || NULL == file_priv) {
		return 0;
	}

	drm_data->rtn = emgd_bc_ts_bridge_release_buffers(dev, drm_data, file_priv);

	EMGD_DEBUG("drm_data->rtn = %d", drm_data->rtn);
	EMGD_DEBUG("Returning 0");
	EMGD_TRACE_EXIT;
	
	return 0;

}
int emgd_bc_ts_set_buffer_info(struct drm_device *dev, void *arg, struct drm_file *file_priv)
{
	emgd_drm_bc_ts_t *drm_data = (emgd_drm_bc_ts_t *)arg;

	EMGD_TRACE_ENTER;
	
	if (NULL == dev || NULL == arg || NULL == file_priv) {
		return 0;
	}

	drm_data->rtn = emgd_bc_ts_bridge_set_buffer_info(dev, drm_data, file_priv);

	EMGD_DEBUG("drm_data->rtn = %d", drm_data->rtn);
	EMGD_DEBUG("Returning 0");
	EMGD_TRACE_EXIT;
	
	return 0;
}
int emgd_bc_ts_get_buffers_count(struct drm_device *dev, void *arg, struct drm_file *file_priv)
{
	emgd_drm_bc_ts_t *drm_data = (emgd_drm_bc_ts_t *)arg;

	EMGD_TRACE_ENTER;
	
	if (NULL == dev || NULL == arg || NULL == file_priv) {
		return 0;
	}

	drm_data->rtn = emgd_bc_ts_bridge_get_buffers_count(dev, drm_data, file_priv);

	EMGD_DEBUG("drm_data->rtn = %d", drm_data->rtn);
	EMGD_DEBUG("Returning 0");
	EMGD_TRACE_EXIT;
	
	return 0;
}
int emgd_bc_ts_get_buffer_index(struct drm_device *dev, void *arg, struct drm_file *file_priv)
{
	emgd_drm_bc_ts_t *drm_data = (emgd_drm_bc_ts_t *)arg;

	EMGD_TRACE_ENTER;
	
	if (NULL == dev || NULL == arg || NULL == file_priv) {
		return 0;
	}

	drm_data->rtn = emgd_bc_ts_bridge_get_buffer_index(dev, drm_data, file_priv);

	EMGD_DEBUG("drm_data->rtn = %d", drm_data->rtn);
	EMGD_DEBUG("Returning 0");
	EMGD_TRACE_EXIT;
	
	return 0;
}

void emgd_bc_ts_set_state(BC_DEVINFO *psDevInfo, const IMG_CHAR state)
{
	if (NULL != psDevInfo) {
		psDevInfo->sBufferInfo.szDeviceName[TSBUFFERCLASS_DEVSTATUS_OFFSET] = state;
	}
}

