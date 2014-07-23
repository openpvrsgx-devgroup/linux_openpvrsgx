/*************************************************************************/ /*!
@Title          Services definitions required by external drivers
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Provides services data structures, defines and prototypes
                required by external drivers.
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

#if !defined (__SERVICESEXT_H__)
#define __SERVICESEXT_H__

#define PVRSRV_LOCKFLG_READONLY     	(1)

/*!
 *****************************************************************************
 * Error values
 *
 * NOTE: If you change this, make sure you update the error texts in
 *   services4/include/pvrsrv_errors.h to match.
 *
 *****************************************************************************/
typedef enum _PVRSRV_ERROR_
{
	PVRSRV_OK								=  0,
	PVRSRV_ERROR_GENERIC					=  1,
	PVRSRV_ERROR_OUT_OF_MEMORY				=  2,
	PVRSRV_ERROR_TOO_FEW_BUFFERS			=  3,
	PVRSRV_ERROR_SYMBOL_NOT_FOUND			=  4,
	PVRSRV_ERROR_OUT_OF_HSPACE				=  5,
	PVRSRV_ERROR_INVALID_PARAMS				=  6,
	PVRSRV_ERROR_TILE_MAP_FAILED			=  7,
	PVRSRV_ERROR_INIT_FAILURE				=  8,
	PVRSRV_ERROR_CANT_REGISTER_CALLBACK 	=  9,
	PVRSRV_ERROR_INVALID_DEVICE				= 10,
	PVRSRV_ERROR_NOT_OWNER					= 11,
	PVRSRV_ERROR_BAD_MAPPING				= 12,
	PVRSRV_ERROR_TIMEOUT					= 13,
	PVRSRV_ERROR_NO_PRIMARY					= 14,
	PVRSRV_ERROR_FLIP_CHAIN_EXISTS			= 15,
	PVRSRV_ERROR_CANNOT_ACQUIRE_SYSDATA 	= 16,
	PVRSRV_ERROR_SCENE_INVALID				= 17,
	PVRSRV_ERROR_STREAM_ERROR				= 18,
	PVRSRV_ERROR_INVALID_INTERRUPT      	= 19,
	PVRSRV_ERROR_FAILED_DEPENDENCIES		= 20,
	PVRSRV_ERROR_CMD_NOT_PROCESSED			= 21,
	PVRSRV_ERROR_CMD_TOO_BIG				= 22,
	PVRSRV_ERROR_DEVICE_REGISTER_FAILED 	= 23,
	PVRSRV_ERROR_FIFO_SPACE					= 24,
	PVRSRV_ERROR_TA_RECOVERY				= 25,
	PVRSRV_ERROR_INDOSORLOWPOWER			= 26,
	PVRSRV_ERROR_TOOMANYBUFFERS				= 27,
	PVRSRV_ERROR_NOT_SUPPORTED				= 28,
	PVRSRV_ERROR_PROCESSING_BLOCKED			= 29,


	PVRSRV_ERROR_CANNOT_FLUSH_QUEUE			= 31,
	PVRSRV_ERROR_CANNOT_GET_QUEUE_SPACE		= 32,
	PVRSRV_ERROR_CANNOT_GET_RENDERDETAILS	= 33,
	PVRSRV_ERROR_RETRY						= 34,

	PVRSRV_ERROR_DDK_VERSION_MISMATCH		= 35,
	PVRSRV_ERROR_BUILD_MISMATCH				= 36,
	PVRSRV_ERROR_PDUMP_BUF_OVERFLOW,

	PVRSRV_ERROR_FORCE_I32 = 0x7fffffff

} PVRSRV_ERROR;


/*!
 *****************************************************************************
 * List of known device classes.
 *****************************************************************************/
typedef enum _PVRSRV_DEVICE_CLASS_
{
	PVRSRV_DEVICE_CLASS_3D				= 0 ,
	PVRSRV_DEVICE_CLASS_DISPLAY			= 1 ,
	PVRSRV_DEVICE_CLASS_BUFFER			= 2 ,
	PVRSRV_DEVICE_CLASS_VIDEO			= 3 ,

	PVRSRV_DEVICE_CLASS_FORCE_I32 		= 0x7fffffff

} PVRSRV_DEVICE_CLASS;


/*!
 *****************************************************************************
 *	States for power management
 *****************************************************************************/
typedef enum _PVRSRV_SYS_POWER_STATE_
{
	PVRSRV_SYS_POWER_STATE_Unspecified		= -1,
	PVRSRV_SYS_POWER_STATE_D0				= 0,
	PVRSRV_SYS_POWER_STATE_D1				= 1,
	PVRSRV_SYS_POWER_STATE_D2				= 2,
	PVRSRV_SYS_POWER_STATE_D3				= 3,
	PVRSRV_SYS_POWER_STATE_D4				= 4,

	PVRSRV_SYS_POWER_STATE_FORCE_I32 = 0x7fffffff

} PVRSRV_SYS_POWER_STATE, *PPVRSRV_SYS_POWER_STATE;


typedef enum _PVRSRV_DEV_POWER_STATE_
{
	PVRSRV_DEV_POWER_STATE_DEFAULT	= -1,
	PVRSRV_DEV_POWER_STATE_ON		= 0,
	PVRSRV_DEV_POWER_STATE_IDLE		= 1,
	PVRSRV_DEV_POWER_STATE_OFF		= 2,

	PVRSRV_DEV_POWER_STATE_FORCE_I32 = 0x7fffffff

} PVRSRV_DEV_POWER_STATE, *PPVRSRV_DEV_POWER_STATE;


/* Power transition handler prototypes */
typedef PVRSRV_ERROR (*PFN_PRE_POWER) (IMG_HANDLE				hDevHandle,
									   PVRSRV_DEV_POWER_STATE	eNewPowerState,
									   PVRSRV_DEV_POWER_STATE	eCurrentPowerState);
typedef PVRSRV_ERROR (*PFN_POST_POWER) (IMG_HANDLE				hDevHandle,
										PVRSRV_DEV_POWER_STATE	eNewPowerState,
										PVRSRV_DEV_POWER_STATE	eCurrentPowerState);

/* Clock speed handler prototypes */
typedef PVRSRV_ERROR (*PFN_PRE_CLOCKSPEED_CHANGE) (IMG_HANDLE				hDevHandle,
												   IMG_BOOL					bIdleDevice,
												   PVRSRV_DEV_POWER_STATE	eCurrentPowerState);
typedef PVRSRV_ERROR (*PFN_POST_CLOCKSPEED_CHANGE) (IMG_HANDLE				hDevHandle,
													IMG_BOOL				bIdleDevice,
													PVRSRV_DEV_POWER_STATE	eCurrentPowerState);


/*****************************************************************************
 * Enumeration of all possible pixel types. Where applicable, Ordering of name
 * is in reverse order of memory bytes (i.e. as a word in little endian).
 * e.g. A8R8G8B8 is in memory as 4 bytes in order: BB GG RR AA
 *
 * NOTE: When modifying this structure please update the client driver format
 *       tables located in %WORKROOT%/eurasia/codegen/pixfmts using the tool
 *       located in %WORKROOT%/eurasia/tools/intern/TextureFormatParser.
 *
 *****************************************************************************/
typedef enum _PVRSRV_PIXEL_FORMAT_ {

	PVRSRV_PIXEL_FORMAT_UNKNOWN			=  0,
	PVRSRV_PIXEL_FORMAT_RGB565			=  1,
	PVRSRV_PIXEL_FORMAT_RGB555			=  2,
	PVRSRV_PIXEL_FORMAT_RGB888			=  3,
	PVRSRV_PIXEL_FORMAT_BGR888			=  4,
	PVRSRV_PIXEL_FORMAT_GREY_SCALE		=  8,
	PVRSRV_PIXEL_FORMAT_PAL12			= 13,
	PVRSRV_PIXEL_FORMAT_PAL8			= 14,
	PVRSRV_PIXEL_FORMAT_PAL4			= 15,
	PVRSRV_PIXEL_FORMAT_PAL2			= 16,
	PVRSRV_PIXEL_FORMAT_PAL1			= 17,
	PVRSRV_PIXEL_FORMAT_ARGB1555		= 18,
	PVRSRV_PIXEL_FORMAT_ARGB4444		= 19,
	PVRSRV_PIXEL_FORMAT_ARGB8888		= 20,
	PVRSRV_PIXEL_FORMAT_ABGR8888		= 21,
	PVRSRV_PIXEL_FORMAT_YV12			= 22,
	PVRSRV_PIXEL_FORMAT_I420			= 23,
    PVRSRV_PIXEL_FORMAT_IMC2            = 25,
	PVRSRV_PIXEL_FORMAT_XRGB8888        = 26,
	PVRSRV_PIXEL_FORMAT_XBGR8888,
	PVRSRV_PIXEL_FORMAT_BGRA8888,
	PVRSRV_PIXEL_FORMAT_XRGB4444,
	PVRSRV_PIXEL_FORMAT_ARGB8332,
	PVRSRV_PIXEL_FORMAT_A2RGB10,
	PVRSRV_PIXEL_FORMAT_A2BGR10,
	PVRSRV_PIXEL_FORMAT_P8,
	PVRSRV_PIXEL_FORMAT_L8,
	PVRSRV_PIXEL_FORMAT_A8L8,
	PVRSRV_PIXEL_FORMAT_A4L4,
	PVRSRV_PIXEL_FORMAT_L16,
	PVRSRV_PIXEL_FORMAT_L6V5U5,
	PVRSRV_PIXEL_FORMAT_V8U8,
	PVRSRV_PIXEL_FORMAT_V16U16,
	PVRSRV_PIXEL_FORMAT_QWVU8888,
	PVRSRV_PIXEL_FORMAT_XLVU8888,
	PVRSRV_PIXEL_FORMAT_QWVU16,
	PVRSRV_PIXEL_FORMAT_D16,
	PVRSRV_PIXEL_FORMAT_D24S8,
	PVRSRV_PIXEL_FORMAT_D24X8,


	PVRSRV_PIXEL_FORMAT_ABGR16,
	PVRSRV_PIXEL_FORMAT_ABGR16F,
	PVRSRV_PIXEL_FORMAT_ABGR32,
	PVRSRV_PIXEL_FORMAT_ABGR32F,
	PVRSRV_PIXEL_FORMAT_B10GR11,
	PVRSRV_PIXEL_FORMAT_GR88,
	PVRSRV_PIXEL_FORMAT_BGR32,
	PVRSRV_PIXEL_FORMAT_GR32,
	PVRSRV_PIXEL_FORMAT_E5BGR9,


	PVRSRV_PIXEL_FORMAT_DUMMY1,
	PVRSRV_PIXEL_FORMAT_DUMMY2,
	PVRSRV_PIXEL_FORMAT_DUMMY3,
	PVRSRV_PIXEL_FORMAT_DUMMY4,
	PVRSRV_PIXEL_FORMAT_DUMMY5,


	PVRSRV_PIXEL_FORMAT_R8G8_B8G8,
	PVRSRV_PIXEL_FORMAT_G8R8_G8B8,


	PVRSRV_PIXEL_FORMAT_NV11,
	PVRSRV_PIXEL_FORMAT_NV12,


	PVRSRV_PIXEL_FORMAT_YUY2,
	PVRSRV_PIXEL_FORMAT_YUV420,
	PVRSRV_PIXEL_FORMAT_YUV444,
	PVRSRV_PIXEL_FORMAT_VUY444,
	PVRSRV_PIXEL_FORMAT_YUYV,
	PVRSRV_PIXEL_FORMAT_YVYU,
	PVRSRV_PIXEL_FORMAT_UYVY,
	PVRSRV_PIXEL_FORMAT_VYUY,

	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY,
	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV,
	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YVYU,
	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_VYUY,
	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_AYUV,


	PVRSRV_PIXEL_FORMAT_A32B32G32R32,
	PVRSRV_PIXEL_FORMAT_A32B32G32R32F,
	PVRSRV_PIXEL_FORMAT_A32B32G32R32_UINT,
	PVRSRV_PIXEL_FORMAT_A32B32G32R32_SINT,


	PVRSRV_PIXEL_FORMAT_B32G32R32,
	PVRSRV_PIXEL_FORMAT_B32G32R32F,
	PVRSRV_PIXEL_FORMAT_B32G32R32_UINT,
	PVRSRV_PIXEL_FORMAT_B32G32R32_SINT,


	PVRSRV_PIXEL_FORMAT_G32R32,
	PVRSRV_PIXEL_FORMAT_G32R32F,
	PVRSRV_PIXEL_FORMAT_G32R32_UINT,
	PVRSRV_PIXEL_FORMAT_G32R32_SINT,


	PVRSRV_PIXEL_FORMAT_D32F,
	PVRSRV_PIXEL_FORMAT_R32,
	PVRSRV_PIXEL_FORMAT_R32F,
	PVRSRV_PIXEL_FORMAT_R32_UINT,
	PVRSRV_PIXEL_FORMAT_R32_SINT,


	PVRSRV_PIXEL_FORMAT_A16B16G16R16,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16F,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16_SINT,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16_SNORM,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16_UINT,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16_UNORM,


	PVRSRV_PIXEL_FORMAT_G16R16,
	PVRSRV_PIXEL_FORMAT_G16R16F,
	PVRSRV_PIXEL_FORMAT_G16R16_UINT,
	PVRSRV_PIXEL_FORMAT_G16R16_UNORM,
	PVRSRV_PIXEL_FORMAT_G16R16_SINT,
	PVRSRV_PIXEL_FORMAT_G16R16_SNORM,


	PVRSRV_PIXEL_FORMAT_R16,
	PVRSRV_PIXEL_FORMAT_R16F,
	PVRSRV_PIXEL_FORMAT_R16_UINT,
	PVRSRV_PIXEL_FORMAT_R16_UNORM,
	PVRSRV_PIXEL_FORMAT_R16_SINT,
	PVRSRV_PIXEL_FORMAT_R16_SNORM,


	PVRSRV_PIXEL_FORMAT_X8R8G8B8,
	PVRSRV_PIXEL_FORMAT_X8R8G8B8_UNORM,
	PVRSRV_PIXEL_FORMAT_X8R8G8B8_UNORM_SRGB,

	PVRSRV_PIXEL_FORMAT_A8R8G8B8,
	PVRSRV_PIXEL_FORMAT_A8R8G8B8_UNORM,
	PVRSRV_PIXEL_FORMAT_A8R8G8B8_UNORM_SRGB,

	PVRSRV_PIXEL_FORMAT_A8B8G8R8,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_UINT,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_UNORM,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_UNORM_SRGB,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_SINT,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_SNORM,


	PVRSRV_PIXEL_FORMAT_G8R8,
	PVRSRV_PIXEL_FORMAT_G8R8_UINT,
	PVRSRV_PIXEL_FORMAT_G8R8_UNORM,
	PVRSRV_PIXEL_FORMAT_G8R8_SINT,
	PVRSRV_PIXEL_FORMAT_G8R8_SNORM,


	PVRSRV_PIXEL_FORMAT_A8,
	PVRSRV_PIXEL_FORMAT_R8,
	PVRSRV_PIXEL_FORMAT_R8_UINT,
	PVRSRV_PIXEL_FORMAT_R8_UNORM,
	PVRSRV_PIXEL_FORMAT_R8_SINT,
	PVRSRV_PIXEL_FORMAT_R8_SNORM,


	PVRSRV_PIXEL_FORMAT_A2B10G10R10,
	PVRSRV_PIXEL_FORMAT_A2B10G10R10_UNORM,
	PVRSRV_PIXEL_FORMAT_A2B10G10R10_UINT,


	PVRSRV_PIXEL_FORMAT_B10G11R11,
	PVRSRV_PIXEL_FORMAT_B10G11R11F,


	PVRSRV_PIXEL_FORMAT_X24G8R32,
	PVRSRV_PIXEL_FORMAT_G8R24,
	PVRSRV_PIXEL_FORMAT_X8R24,
	PVRSRV_PIXEL_FORMAT_E5B9G9R9,
	PVRSRV_PIXEL_FORMAT_R1,

	PVRSRV_PIXEL_FORMAT_DUMMY6,
	PVRSRV_PIXEL_FORMAT_DUMMY7,
	PVRSRV_PIXEL_FORMAT_DUMMY8,
	PVRSRV_PIXEL_FORMAT_DUMMY9,
	PVRSRV_PIXEL_FORMAT_DUMMY10,
	PVRSRV_PIXEL_FORMAT_DUMMY11,
	PVRSRV_PIXEL_FORMAT_DUMMY12,
	PVRSRV_PIXEL_FORMAT_DUMMY13,
	PVRSRV_PIXEL_FORMAT_DUMMY14,
	PVRSRV_PIXEL_FORMAT_DUMMY15,
	PVRSRV_PIXEL_FORMAT_DUMMY16,
	PVRSRV_PIXEL_FORMAT_DUMMY17,
	PVRSRV_PIXEL_FORMAT_DUMMY18,
	PVRSRV_PIXEL_FORMAT_DUMMY19,
	PVRSRV_PIXEL_FORMAT_DUMMY20,


	PVRSRV_PIXEL_FORMAT_L_F16,
	PVRSRV_PIXEL_FORMAT_L_F16_REP,
	PVRSRV_PIXEL_FORMAT_L_F16_A_F16,
	PVRSRV_PIXEL_FORMAT_A_F16,
	PVRSRV_PIXEL_FORMAT_B16G16R16F,

	PVRSRV_PIXEL_FORMAT_L_F32,
	PVRSRV_PIXEL_FORMAT_A_F32,
	PVRSRV_PIXEL_FORMAT_L_F32_A_F32,


	PVRSRV_PIXEL_FORMAT_PVRTC2,
	PVRSRV_PIXEL_FORMAT_PVRTC4,
	PVRSRV_PIXEL_FORMAT_PVRTCII2,
	PVRSRV_PIXEL_FORMAT_PVRTCII4,
	PVRSRV_PIXEL_FORMAT_PVRTCIII,
	PVRSRV_PIXEL_FORMAT_PVRO8,
	PVRSRV_PIXEL_FORMAT_PVRO88,
	PVRSRV_PIXEL_FORMAT_PT1,
	PVRSRV_PIXEL_FORMAT_PT2,
	PVRSRV_PIXEL_FORMAT_PT4,
	PVRSRV_PIXEL_FORMAT_PT8,
	PVRSRV_PIXEL_FORMAT_PTW,
	PVRSRV_PIXEL_FORMAT_PTB,
	PVRSRV_PIXEL_FORMAT_MONO8,
	PVRSRV_PIXEL_FORMAT_MONO16,


	PVRSRV_PIXEL_FORMAT_C0_YUYV,
	PVRSRV_PIXEL_FORMAT_C0_UYVY,
	PVRSRV_PIXEL_FORMAT_C0_YVYU,
	PVRSRV_PIXEL_FORMAT_C0_VYUY,
	PVRSRV_PIXEL_FORMAT_C1_YUYV,
	PVRSRV_PIXEL_FORMAT_C1_UYVY,
	PVRSRV_PIXEL_FORMAT_C1_YVYU,
	PVRSRV_PIXEL_FORMAT_C1_VYUY,


	PVRSRV_PIXEL_FORMAT_C0_YUV420_2P_UV,
	PVRSRV_PIXEL_FORMAT_C0_YUV420_2P_VU,
	PVRSRV_PIXEL_FORMAT_C0_YUV420_3P,
	PVRSRV_PIXEL_FORMAT_C1_YUV420_2P_UV,
	PVRSRV_PIXEL_FORMAT_C1_YUV420_2P_VU,
	PVRSRV_PIXEL_FORMAT_C1_YUV420_3P,

	PVRSRV_PIXEL_FORMAT_A2B10G10R10F,
	PVRSRV_PIXEL_FORMAT_B8G8R8_SINT,
	PVRSRV_PIXEL_FORMAT_PVRF32SIGNMASK,

	PVRSRV_PIXEL_FORMAT_FORCE_I32 = 0x7fffffff,
} PVRSRV_PIXEL_FORMAT;

/*!
 *****************************************************************************
 * Enumeration of possible alpha types.
 *****************************************************************************/
typedef enum _PVRSRV_ALPHA_FORMAT_ {
	PVRSRV_ALPHA_FORMAT_UNKNOWN		=  0x00000000,
	PVRSRV_ALPHA_FORMAT_PRE			=  0x00000001,
	PVRSRV_ALPHA_FORMAT_NONPRE		=  0x00000002,
	PVRSRV_ALPHA_FORMAT_MASK		=  0x0000000F,
} PVRSRV_ALPHA_FORMAT;

/*!
 *****************************************************************************
 * Enumeration of possible alpha types.
 *****************************************************************************/
typedef enum _PVRSRV_COLOURSPACE_FORMAT_ {
	PVRSRV_COLOURSPACE_FORMAT_UNKNOWN		=  0x00000000,
	PVRSRV_COLOURSPACE_FORMAT_LINEAR		=  0x00010000,
	PVRSRV_COLOURSPACE_FORMAT_NONLINEAR		=  0x00020000,
	PVRSRV_COLOURSPACE_FORMAT_MASK			=  0x000F0000,
} PVRSRV_COLOURSPACE_FORMAT;


/*
 * Drawable orientation (in degrees clockwise).
 * Opposite sense from WSEGL.
 */
typedef enum _PVRSRV_ROTATION_ {
	PVRSRV_ROTATE_0		=	0,
	PVRSRV_ROTATE_90	=	1,
	PVRSRV_ROTATE_180	=	2,
	PVRSRV_ROTATE_270	=	3,
	PVRSRV_FLIP_Y

} PVRSRV_ROTATION;

/*!
 * Flags for DisplayClassCreateSwapChain.
 */
#define PVRSRV_CREATE_SWAPCHAIN_SHARED		(1<<0)
#define PVRSRV_CREATE_SWAPCHAIN_QUERY		(1<<1)
#define PVRSRV_CREATE_SWAPCHAIN_OEMOVERLAY	(1<<2)

/*!
 *****************************************************************************
 * Structure providing implementation details for serialisation and
 * synchronisation of operations. This is the fundamental unit on which operations
 * are synced, and would typically be included in any data structures that require
 * serialised accesses etc. e.g. MEM_INFO structures
 *
 *****************************************************************************/
/*
	Sync Data to be shared/mapped between user/kernel
*/
typedef struct _PVRSRV_SYNC_DATA_
{
	/* CPU accessible WriteOp Info */
	IMG_UINT32					ui32WriteOpsPending;
	volatile IMG_UINT32			ui32WriteOpsComplete;

	/* CPU accessible ReadOp Info */
	IMG_UINT32					ui32ReadOpsPending;
	volatile IMG_UINT32			ui32ReadOpsComplete;


	IMG_UINT32					ui32LastOpDumpVal;
	IMG_UINT32					ui32LastReadOpDumpVal;

} PVRSRV_SYNC_DATA;

typedef struct _PVRSRV_CLIENT_SYNC_INFO_
{

	PVRSRV_SYNC_DATA			*psSyncData;





	IMG_DEV_VIRTADDR		sWriteOpsCompleteDevVAddr;


	IMG_DEV_VIRTADDR		sReadOpsCompleteDevVAddr;


	IMG_HANDLE					hMappingInfo;


	IMG_HANDLE					hKernelSyncInfo;

} PVRSRV_CLIENT_SYNC_INFO, *PPVRSRV_CLIENT_SYNC_INFO;

/*!
 *****************************************************************************
 * Resource locking structure
 *****************************************************************************/
typedef struct PVRSRV_RESOURCE_TAG
{
	volatile IMG_UINT32 ui32Lock;
	IMG_UINT32 			ui32ID;
}PVRSRV_RESOURCE;
typedef PVRSRV_RESOURCE PVRSRV_RES_HANDLE;


typedef IMG_VOID (*PFN_CMD_COMPLETE) (IMG_HANDLE);
typedef IMG_VOID (**PPFN_CMD_COMPLETE) (IMG_HANDLE);

typedef IMG_BOOL (*PFN_CMD_PROC) (IMG_HANDLE, IMG_UINT32, IMG_VOID*);
typedef IMG_BOOL (**PPFN_CMD_PROC) (IMG_HANDLE, IMG_UINT32, IMG_VOID*);


typedef struct _IMG_RECT_
{
	IMG_INT32	x0;
	IMG_INT32	y0;
	IMG_INT32	x1;
	IMG_INT32	y1;
}IMG_RECT;

typedef struct _IMG_RECT_16_
{
	IMG_INT16	x0;
	IMG_INT16	y0;
	IMG_INT16	x1;
	IMG_INT16	y1;
}IMG_RECT_16;


/* common pfn between BC/DC */
typedef PVRSRV_ERROR (*PFN_GET_BUFFER_ADDR)(IMG_HANDLE,
											IMG_HANDLE,
											IMG_SYS_PHYADDR**,
											IMG_SIZE_T*,
											IMG_VOID**,
											IMG_HANDLE*,
											IMG_BOOL*,
											IMG_UINT32*);

typedef PVRSRV_ERROR (*PFN_GET_BUFFER_ID_FROM_TAG)(IMG_HANDLE, IMG_UINT32, IMG_HANDLE);

typedef struct DISPLAY_DIMS_TAG
{
	IMG_UINT32	ui32ByteStride;
	IMG_UINT32	ui32Width;
	IMG_UINT32	ui32Height;
} DISPLAY_DIMS;


typedef struct DISPLAY_FORMAT_TAG
{
	/* pixel format type */
	PVRSRV_PIXEL_FORMAT		pixelformat;
} DISPLAY_FORMAT;

typedef struct DISPLAY_SURF_ATTRIBUTES_TAG
{
	/* pixel format type */
	PVRSRV_PIXEL_FORMAT		pixelformat;
	/* dimensions information structure array */
	DISPLAY_DIMS			sDims;
	IMG_UINT32			ui32Reseved;
} DISPLAY_SURF_ATTRIBUTES;


typedef struct DISPLAY_MODE_INFO_TAG
{
	/* pixel format type */
	PVRSRV_PIXEL_FORMAT		pixelformat;
	/* dimensions information structure array */
	DISPLAY_DIMS			sDims;
	/* refresh rate of the display */
	IMG_UINT32				ui32RefreshHZ;
	/* OEM specific flags */
	IMG_UINT32				ui32OEMFlags;
} DISPLAY_MODE_INFO;



#define MAX_DISPLAY_NAME_SIZE	(50)

typedef struct DISPLAY_INFO_TAG
{
	/* max swapchains supported */
	IMG_UINT32 ui32MaxSwapChains;
	/* max buffers in a swapchain */
	IMG_UINT32 ui32MaxSwapChainBuffers;
	/* min swap interval supported */
	IMG_UINT32 ui32MinSwapInterval;
	/* max swap interval supported */
	IMG_UINT32 ui32MaxSwapInterval;
	/* physical dimensions of the display required for DPI calc. */
	IMG_UINT32 ui32PhysicalWidthmm;
	IMG_UINT32 ui32PhysicalHeightmm;
	/* display name */
	IMG_CHAR	szDisplayName[MAX_DISPLAY_NAME_SIZE];
#if defined(SUPPORT_HW_CURSOR)
	/* cursor dimensions */
	IMG_UINT16	ui32CursorWidth;
	IMG_UINT16	ui32CursorHeight;
#endif
} DISPLAY_INFO;

typedef struct ACCESS_INFO_TAG
{
	IMG_UINT32		ui32Size;
	IMG_UINT32  	ui32FBPhysBaseAddress;
	IMG_UINT32		ui32FBMemAvailable;			/* size of usable FB memory */
	IMG_UINT32  	ui32SysPhysBaseAddress;
	IMG_UINT32		ui32SysSize;
	IMG_UINT32		ui32DevIRQ;
}ACCESS_INFO;


typedef struct PVRSRV_CURSOR_SHAPE_TAG
{
	IMG_UINT16			ui16Width;
	IMG_UINT16			ui16Height;
	IMG_INT16			i16XHot;
	IMG_INT16			i16YHot;


	IMG_VOID*   		pvMask;
	IMG_INT16  			i16MaskByteStride;


	IMG_VOID*			pvColour;
	IMG_INT16			i16ColourByteStride;
	PVRSRV_PIXEL_FORMAT	eColourPixelFormat;
} PVRSRV_CURSOR_SHAPE;

#define PVRSRV_SET_CURSOR_VISIBILITY	(1<<0)
#define PVRSRV_SET_CURSOR_POSITION		(1<<1)
#define PVRSRV_SET_CURSOR_SHAPE			(1<<2)
#define PVRSRV_SET_CURSOR_ROTATION		(1<<3)

typedef struct PVRSRV_CURSOR_INFO_TAG
{

	IMG_UINT32 ui32Flags;


	IMG_BOOL bVisible;


	IMG_INT16 i16XPos;
	IMG_INT16 i16YPos;


	PVRSRV_CURSOR_SHAPE sCursorShape;


	IMG_UINT32 ui32Rotation;

} PVRSRV_CURSOR_INFO;


typedef struct _PVRSRV_REGISTRY_INFO_
{
    IMG_UINT32		ui32DevCookie;
    IMG_PCHAR		pszKey;
    IMG_PCHAR		pszValue;
    IMG_PCHAR		pszBuf;
    IMG_UINT32		ui32BufSize;
} PVRSRV_REGISTRY_INFO, *PPVRSRV_REGISTRY_INFO;


PVRSRV_ERROR IMG_CALLCONV PVRSRVReadRegistryString (PPVRSRV_REGISTRY_INFO psRegInfo);
PVRSRV_ERROR IMG_CALLCONV PVRSRVWriteRegistryString (PPVRSRV_REGISTRY_INFO psRegInfo);


#define PVRSRV_BC_FLAGS_YUVCSC_CONFORMANT_RANGE	(0 << 0)
#define PVRSRV_BC_FLAGS_YUVCSC_FULL_RANGE		(1 << 0)

#define PVRSRV_BC_FLAGS_YUVCSC_BT601			(0 << 1)
#define PVRSRV_BC_FLAGS_YUVCSC_BT709			(1 << 1)

#define MAX_BUFFER_DEVICE_NAME_SIZE	(50) /* arbitrary choice! */

/* buffer information structure */
typedef struct BUFFER_INFO_TAG
{
	IMG_UINT32 			ui32BufferCount;
	IMG_UINT32			ui32BufferDeviceID;
	PVRSRV_PIXEL_FORMAT	pixelformat;
	IMG_UINT32			ui32ByteStride;
	IMG_UINT32			ui32Width;
	IMG_UINT32			ui32Height;
	IMG_UINT32			ui32Flags;
	IMG_CHAR			szDeviceName[MAX_BUFFER_DEVICE_NAME_SIZE];
} BUFFER_INFO;

typedef enum _OVERLAY_DEINTERLACE_MODE_
{
	WEAVE=0x0,
	BOB_ODD,
	BOB_EVEN,
	BOB_EVEN_NONINTERLEAVED
} OVERLAY_DEINTERLACE_MODE;

#endif /* __SERVICESEXT_H__ */
/*****************************************************************************
 End of file (servicesext.h)
*****************************************************************************/
