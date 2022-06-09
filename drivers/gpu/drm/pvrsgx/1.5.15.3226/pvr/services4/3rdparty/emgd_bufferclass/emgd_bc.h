/**********************************************************************
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

#ifndef __EMGD_BC__H__
#define __EMGD_BC__H__

#include "img_defs.h"
#include "servicesext.h"
#include "kernelbuffer.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    enum BC_memory
    {
        BC_MEMORY_MMAP = 1,
        BC_MEMORY_USERPTR = 2,
    };

    /* 
     * the following types are tested for fourcc in struct bc_buf_params_t
     *   NV12
     *   UYVY
     *   RGB565 - not tested yet
     *   YUYV
     */
    typedef struct bc_buf_params
    {
        int count;            /*number of buffers, [in/out] */
        int width;            /*buffer width in pixel, multiple of 8 or 32 */
        int height;            /*buffer height in pixel */
        int stride;
        unsigned int fourcc;    /*buffer pixel format */
        enum BC_memory type;
    } bc_buf_params_t;

    extern IMG_IMPORT IMG_BOOL
        PVRGetBufferClassJTable (PVRSRV_BC_BUFFER2SRV_KMJTABLE * psJTable);

#define BUFCLASS_DEVICE_MAX_ID	6
#define BUFCLASS_BUFFER_MAX		64
/* 
** Device Name Space is for device name, steam tag and device state.
** Format
** [Name]...[Video ID][Device Status]
** From Device Name Space tail, 
** Device Status - one Byte
** Video ID - four Byte
** Device Name - variation according to device name set.
*/
#define TSBUFFERCLASS_DEV_NAME_LEN (MAX_BUFFER_DEVICE_NAME_SIZE - sizeof(IMG_UINT32) - sizeof(IMG_CHAR) - 1)
#define TSBUFFERCLASS_VIDEOID_OFFSET (MAX_BUFFER_DEVICE_NAME_SIZE - sizeof(IMG_UINT32) - sizeof(IMG_CHAR))
#define TSBUFFERCLASS_DEVSTATUS_OFFSET (MAX_BUFFER_DEVICE_NAME_SIZE - sizeof(IMG_CHAR))

    typedef void *BCE_HANDLE;

    typedef enum tag_bce_bool
    {
        BCE_FALSE = 0,
        BCE_TRUE = 1,
    } BCE_BOOL, *BCE_PBOOL;

    typedef struct BC_BUFFER_TAG
    {
        IMG_UINT32 ulSize;
        IMG_HANDLE hMemHandle;

        IMG_SYS_PHYADDR *psSysAddr;
		IMG_UINT32 SysAddr;

        IMG_CPU_VIRTADDR sCPUVAddr;
        PVRSRV_SYNC_DATA *psSyncData;

        struct BC_BUFFER_TAG *psNext;
        IMG_UINT32 sBufferHandle;
        IMG_BOOL is_conti_addr;
		IMG_UINT32 tag; /* Buffer Tag. -- Surface ID*/
		IMG_BOOL mapped;
    } BC_BUFFER;

    typedef struct BC_DEVINFO_TAG
    {
        BC_BUFFER *psSystemBuffer;
        PVRSRV_BC_BUFFER2SRV_KMJTABLE sPVRJTable;
        PVRSRV_BC_SRV2BUFFER_KMJTABLE sBCJTable;
        BCE_HANDLE hPVRServices;
        IMG_UINT32 ulRefCount;
        BUFFER_INFO sBufferInfo;
        enum BC_memory buf_type;
        IMG_UINT32 Device_ID;
    } BC_DEVINFO;

    typedef enum emgd_bc_error_
    {
        EMGD_OK = 0,
        EMGD_ERROR_GENERIC = 1,
        EMGD_ERROR_OUT_OF_MEMORY = 2,
        EMGD_ERROR_TOO_FEW_BUFFERS = 3,
        EMGD_ERROR_INVALID_PARAMS = 4,
        EMGD_ERROR_INIT_FAILURE = 5,
        EMGD_ERROR_CANT_REGISTER_CALLBACK = 6,
        EMGD_ERROR_INVALID_DEVICE = 7,
        EMGD_ERROR_DEVICE_REGISTER_FAILED = 8,
        EMGD_ERROR_NO_PRIMARY = 9
    } emgd_error_t;

#ifndef UNREFERENCED_PARAMETER
#define    UNREFERENCED_PARAMETER(param) (param) = (param)
#endif
#if 0
#ifndef NULL
#define NULL 0
#endif
#endif
emgd_error_t bc_ts_init(IMG_UINT32 id);
emgd_error_t bc_ts_uninit(IMG_UINT32 id);

emgd_error_t BCOpenPVRServices(BCE_HANDLE * phPVRServices);
emgd_error_t BCClosePVRServices(BCE_HANDLE hPVRServices);

void *BCAllocKernelMem(unsigned long ulSize);
void BCFreeKernelMem(void *pvMem);
#if 0
    BCE_ERROR BCAllocDiscontigMemory(unsigned long ulSize,
            BCE_HANDLE unref__ * phMemHandle,
            IMG_CPU_VIRTADDR * pLinAddr,
            IMG_SYS_PHYADDR ** ppPhysAddr);

    void BCFreeDiscontigMemory(unsigned long ulSize,
            BCE_HANDLE unref__ hMemHandle,
            IMG_CPU_VIRTADDR LinAddr,
            IMG_SYS_PHYADDR * pPhysAddr);

    IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC(IMG_CPU_PHYADDR cpu_paddr);
    IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC(IMG_SYS_PHYADDR sys_paddr);

    void *MapPhysAddr(IMG_SYS_PHYADDR sSysAddr, unsigned long ulSize);
    void UnMapPhysAddr(void *pvAddr, unsigned long ulSize);
#endif
emgd_error_t BCGetLibFuncAddr(BCE_HANDLE hExtDrv, char *szFunctionName,
            PFN_BC_GET_PVRJTABLE * ppfnFuncTable);
BC_DEVINFO *GetAnchorPtr(int id);

void bc_ts_free_bcbuffer(BC_BUFFER *bc_buf);

#if defined(__cplusplus)
}
#endif

#endif
