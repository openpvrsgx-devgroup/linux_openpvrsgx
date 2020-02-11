/*************************************************************************/ /*!
@Title          Poulsbo Display kernel driver structures and prototypes
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Poulsbo Display kernel driver structures and prototypes
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
#if !defined(__DC_POULSBO_H__)
#define __DC_POULSBO_H__

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "poulsbo_regs.h"

#if defined(SUPPORT_DRI_DRM)
#include <linux/version.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_helper.h>
#endif /* defined(SUPPORT_DRI_DRM) */

#include <linux/pci.h>

#define	MAKESTRING(x) #x
#define TOSTRING(x) MAKESTRING(x)

#if !defined(DISPLAY_CONTROLLER)
#define DISPLAY_CONTROLLER dc_poulsbo
#endif

#define	DRVNAME	TOSTRING(DISPLAY_CONTROLLER)

#define DISPLAY_DEVICE_NAME "DC_POULSBO"

#if defined(SUPPORT_DRI_DRM)
#if defined(PVRPSB_WIDTH) && !defined(PVRPSB_HEIGHT)
#error ERROR: PVRPSB_HEIGHT not defined
#elif !defined(PVRPSB_WIDTH) && defined(PVRPSB_HEIGHT)
#error ERROR: PVRPSB_WIDTH not defined
#endif
#endif /* #if defined(SUPPORT_DRI_DRM) */


#if !defined(MAX)
#define MAX(a, b)				((a) > (b) ? (a) : (b))
#endif

#if !defined(MIN)
#define MIN(a, b)				((a) < (b) ? (a) : (b))
#endif

#if !defined(ABS)
#define ABS(v)					(((v) > 0) ? (v) : -(v))
#endif

#define PVRPSB_ALIGN(value, align)		(((value) + ((align) - 1)) & ~((align) - 1))


#define PVRPSB_MAX_FORMATS			(1)
#define PVRPSB_MAX_DIMS				(1)
#define PVRPSB_MAX_SWAPCHAINS			(1)

#define PVRPSB_MIN_SWAP_INTERVAL		(0)
#define PVRPSB_MAX_SWAP_INTERVAL		(10) /* Arbitary choice */

#if defined(USE_PRIMARY_SURFACE_IN_FLIP_CHAIN)
#define PVRPSB_MAX_BACKBUFFERS			(2)
#else
#define PVRPSB_MAX_BACKBUFFERS			(3)
#endif

#define PVRPSB_FB_WIDTH_MIN			(0)
#define PVRPSB_FB_WIDTH_MAX			(2048)
#define PVRPSB_FB_HEIGHT_MIN			(0)
#define PVRPSB_FB_HEIGHT_MAX			(2048)

#define PVRPSB_HW_CURSOR_WIDTH			(64)
#define PVRPSB_HW_CURSOR_HEIGHT			(64)
#define PVRPSB_HW_CURSOR_STRIDE			(PVRPSB_HW_CURSOR_WIDTH * 4)	/* Support ARGB */

#define PVRPSB_PAGE_SHIFT			(12)
#define PVRPSB_PAGE_SIZE			(1UL << PVRPSB_PAGE_SHIFT)
#define PVRPSB_PAGE_MASK			(~(PVRPSB_PAGE_SIZE - 1))
#define PVRPSB_PAGE_ALIGN(addr)			PVRPSB_ALIGN((addr), PVRPSB_PAGE_SIZE)

/******************************************************************************
 * Device information
 *****************************************************************************/

/* Fixed Bus/DeviceID/Func for accessing GMA500 PCI Config Space within the
   Intel SCH (which comprises other devices too - not just the GMA500) */
#define PVRPSB_BUS_ID				(0)
#define PVRPSB_DEV_ID				(2)
#define PVRPSB_FUNC				(0)

#define PVRPSB_PCIREG_OFFSET			(0)

/* Names of Registers in GMA500's PCI Config Space */
#define PVRPSB_PCIREG_PCICMD			(0x04)
#define PVRPSB_PCIREG_MEM_BASE			(0x10)
#define PVRPSB_PCIREG_GMEM_BASE			(0x18)
#define PVRPSB_PCIREG_GTT_BASE			(0x1C)
#define PVRPSB_PCIREG_GC			(0x52)
#define PVRPSB_PCIREG_BSM			(0x5C)
#define PVRPSB_PCIREG_MSAC			(0x62)
#define PVRPSB_PCIREG_ASLS			(0xFC)

/* PSB GMA Registers (MEM_BASE) */
#define PVRPSB_PCI_BAR_IDX_MEM_BASE		(0)

/* PSB IO Registers (IO_BASE) */
#define PVRPSB_PCI_BAR_IDX_IO_BASE		(1)

/* PSB Graphics Aperture (GMEM_BASE) */
#define PVRPSB_PCI_BAR_IDX_GMEM_BASE		(2)

/* PSB IOMMU (GTT_BASE) */
#define PVRPSB_PCI_BAR_IDX_GTT_BASE		(3)

/******************************************************************************
 * I2C information
 *****************************************************************************/
#define PVRPSB_I2C_ADDR_SDVO_B			(0x38)

/* I2C message buffers consist of a byte pair. The 
   first byte should be one of the values below. */
#define PVRPSB_I2C_ARG7				(0x00)
#define PVRPSB_I2C_ARG6				(0x01)
#define PVRPSB_I2C_ARG5				(0x02)
#define PVRPSB_I2C_ARG4				(0x03)
#define PVRPSB_I2C_ARG3				(0x04)
#define PVRPSB_I2C_ARG2				(0x05)
#define PVRPSB_I2C_ARG1				(0x06)
#define PVRPSB_I2C_ARG0				(0x07)

#define PVRPSB_I2C_CMD				(0x08)
#define PVRPSB_I2C_STATUS			(0x09)

#define PVRPSB_I2C_RETURN0			(0x0A)
#define PVRPSB_I2C_RETURN1			(0x0B)
#define PVRPSB_I2C_RETURN2			(0x0C)
#define PVRPSB_I2C_RETURN3			(0x0D)
#define PVRPSB_I2C_RETURN4			(0x0E)
#define PVRPSB_I2C_RETURN5			(0x0F)
#define PVRPSB_I2C_RETURN6			(0x10)
#define PVRPSB_I2C_RETURN7			(0x11)

/* If the first I2C message buffer byte is PVRPSB_I2C_CMD 
   then the second byte should be one of the values below. */
#define PVRPSB_I2C_CMD_RESET			(0x01)
#define PVRPSB_I2C_CMD_GETDEVICECAPS		(0x02)
#define PVRPSB_I2C_CMD_GETTRAINEDINPUTS		(0x03)
#define PVRPSB_I2C_CMD_GETACTIVEENCODERS	(0x04)
#define PVRPSB_I2C_CMD_SETACTIVEENCODERS	(0x05)
#define PVRPSB_I2C_CMD_GETINOUTMAP		(0x06)
#define PVRPSB_I2C_CMD_SETINOUTMAP		(0x07)
#define PVRPSB_I2C_CMD_GETATTACHEDDISPLAYS	(0x0B)
#define PVRPSB_I2C_CMD_SETTARGETINPUT		(0x10)
#define PVRPSB_I2C_CMD_SETTARGETOUTPUT		(0x11)
#define PVRPSB_I2C_CMD_GETINPUTTIMINGS1		(0x12)
#define PVRPSB_I2C_CMD_GETINPUTTIMINGS2		(0x13)
#define PVRPSB_I2C_CMD_SETINPUTTIMINGS1		(0x14)
#define PVRPSB_I2C_CMD_SETINPUTTIMINGS2		(0x15)
#define PVRPSB_I2C_CMD_GETOUTPUTTIMINGS1	(0x16)
#define PVRPSB_I2C_CMD_GETOUTPUTTIMINGS2	(0x17)
#define PVRPSB_I2C_CMD_SETOUTPUTTIMINGS1	(0x18)
#define PVRPSB_I2C_CMD_SETOUTPUTTIMINGS2	(0x19)
#define PVRPSB_I2C_CMD_GETCLOCKMULTI		(0x20)
#define PVRPSB_I2C_CMD_SETCLOCKMULTI		(0x21)
#define PVRPSB_I2C_CMD_BUSSWITCH		(0x7A)

/* Encoder types */
#define PVRPSB_ENCODER_NONE			(0)
#define PVRPSB_ENCODER_TMDS0			(1 << 0)
#define PVRPSB_ENCODER_RGB0			(1 << 1)
#define PVRPSB_ENCODER_CVBS0			(1 << 2)
#define PVRPSB_ENCODER_SVID0			(1 << 3)
#define PVRPSB_ENCODER_YPRPB0			(1 << 4)
#define PVRPSB_ENCODER_SCART0			(1 << 5)
#define PVRPSB_ENCODER_LVDS0			(1 << 6)
#define PVRPSB_ENCODER_TMDS1			(1 << 8)
#define PVRPSB_ENCODER_RGB1			(1 << 9)
#define PVRPSB_ENCODER_CVBS1			(1 << 10)
#define PVRPSB_ENCODER_SVID1			(1 << 11)
#define PVRPSB_ENCODER_YPRPB1			(1 << 12)
#define PVRPSB_ENCODER_SCART1			(1 << 13)
#define PVRPSB_ENCODER_LVDS1			(1 << 14)

#if defined(__cplusplus)
extern "C" {
#endif

/******************************************************************************
 * Structs and enums
 *****************************************************************************/

typedef void *PSB_HANDLE;

typedef enum PSB_BOOL_TAG
{
	PSB_FALSE = 0,
	PSB_TRUE  = 1,
} PSB_BOOL, *PSB_PBOOL;

typedef enum PSB_ERROR_TAG
{
	PSB_OK					= 0,
	PSB_ERROR_GENERIC			= 1,
	PSB_ERROR_OUT_OF_MEMORY			= 2,
	PSB_ERROR_TOO_FEW_BUFFERS		= 3,
	PSB_ERROR_INVALID_PARAMS		= 4,
	PSB_ERROR_INIT_FAILURE			= 5,
	PSB_ERROR_CANT_REGISTER_CALLBACK	= 6,
	PSB_ERROR_INVALID_DEVICE		= 7,
	PSB_ERROR_DEVICE_REGISTER_FAILED 	= 8
} PSB_ERROR;

typedef enum PVRPSB_DEVICE_TAG
{
	PSB_UNKNOWN	= 0,
	PSB_POULSBO	= 1,
	PSB_CEDARVIEW	= 2
} PVRPSB_DEVICE;

typedef enum PVRPSB_PORT_TAG
{
	PSB_PORT_ANALOG	= 0,
	PSB_PORT_SDVO_B	= 1,
	PSB_PORT_SDVO_C	= 2,
	PSB_PORT_LVDS	= 3
} PVRPSB_PORT;

typedef enum PVRPSB_PIPE_TAG
{
	PSB_PIPE_A	= 0,
	PSB_PIPE_B	= 1
} PVRPSB_PIPE;


#if defined(SUPPORT_DRI_DRM)
typedef struct __attribute__ ((__packed__)) PVRPSB_DEVICE_CAPS_TAG
{
	IMG_UINT8 ui8VendorID;
	IMG_UINT8 ui8DeviceID;
	IMG_UINT8 ui8DeviceRevisionID;
	IMG_UINT8 ui8VersionMajor;
	IMG_UINT8 ui8VersionMinor;

	IMG_UINT8 inputMask:2;
	IMG_UINT8 smoothScaling:1;
	IMG_UINT8 sharpScaling:1;
	IMG_UINT8 upScaling:1;
	IMG_UINT8 downScaling:1;
	IMG_UINT8 stallSupport:1;
	IMG_UINT8 padding:1;

	IMG_UINT16 ui16OutputFlags;
} PVRPSB_DEVICE_CAPS;

typedef struct __attribute__ ((__packed__)) PVRPSB_TIMINGS1_TAG
{
	IMG_UINT16	ui16Clock;		/* 15-0: Dot clock divided by 10 */
	IMG_UINT8	ui8WidthLower;		/*  7-0: Lower 8 bits of the width (horizontal active) */
	IMG_UINT8	ui8HBlankLower;		/*  7-0: Lower 8 bits of the horizontal blank period/length */
	IMG_UINT8	ui8WidthHBlankHigh;	/*  7-4: Upper 4 bits of the width
						    3-0: Upper 4 bits of the horizontal blank period/length */
	IMG_UINT8	ui8HeightLower;		/*  7-0: Lower 8 bits of the height (vertical active) */
	IMG_UINT8	ui8VBlankLower;		/*  7-0: Lower 8 bits of the vertical blank period/length */
	IMG_UINT8	ui8HeightVBlankHigh;	/*  7-4: Upper 4 bits of the height
						    3-0: Upper 4 bits of the vertical blank period/length */
} PVRPSB_TIMINGS1;

typedef struct __attribute__ ((__packed__)) PVRPSB_TIMINGS2_TAG
{
	IMG_UINT8	ui8HFrontPorchLower;	/* 7-0: Lower 8 bits of the horizontal front porch period  */
	IMG_UINT8	ui8HSyncLower;		/* 7-0: Lower 8 bits of the horizontal sync period */
	IMG_UINT8	ui8VFrontPorchSync;	/* 7-4: Lower 4 bits of the vertical front porch period
						   3-0: Lower 4 bits of the vertical sync period */
	IMG_UINT8	ui8FrontPorchSyncHigh;	/* 7-6: Upper 2 bits of the horizontal front porch period
						   5-4: Upper 2 bits of the horizontal sync period
						   3-2: Next 2 bits of the vertical front porch period
						   1-0: Upper 2 bits of the vertical sync period */
	IMG_UINT8	ui8DTDFlags;		/* 7-0: Detailed Timing Descriptor flags */
	IMG_UINT8	ui8SDVOFlags;		/* 7-0: SDVO flags */
	IMG_UINT8	ui8VFrontPorchHigh;	/* 7-6: Upper 2 bits of the vertical front porch period
						   5-0: Should be set to 0 */
	IMG_UINT8	ui8Padding;		/* 7-0: This is used to pad the structure to 2 DWords (it should be set to 0) */
} PVRPSB_TIMINGS2;

typedef struct __attribute__ ((__packed__)) PVRPSB_TARGET_INTPUT_TAG
{
	IMG_UINT8	target:1;
	IMG_UINT8	padding:7;
} PVRPSB_TARGET_INPUT;


typedef struct PVRPSB_STATE_TAG
{
	struct
	{
		IMG_UINT16 ui16GraphicsControl;
		IMG_UINT32 ui32StolenMemBase;
		IMG_UINT32 ui32ASLStorage;
	} sPciRegisters;

	struct
	{
		IMG_UINT32 ui32PgTblValue;
		IMG_UINT32 *pui32GTTContents;
		
		IMG_UINT32 ui32OvAdd;
		IMG_UINT32 ui32OGamC0;
		IMG_UINT32 ui32OGamC1;
		IMG_UINT32 ui32OGamC2;
		IMG_UINT32 ui32OGamC3;
		IMG_UINT32 ui32OGamC4;
		IMG_UINT32 ui32OGamC5;
		
		IMG_UINT32 ui32DspArb;
		IMG_UINT32 ui32DspFW1;
		IMG_UINT32 ui32DspFW2;
		IMG_UINT32 ui32DspFW3;
		IMG_UINT32 ui32DspFW4;
		IMG_UINT32 ui32DspFW5;
		IMG_UINT32 ui32DspFW6;		
		IMG_UINT32 ui32DspChicken;
	} sDevRegisters;
} PVRPSB_STATE;
#endif /* #if defined(SUPPORT_DRI_DRM) */

typedef struct PVRPSB_BUFFER_TAG
{
	IMG_UINT32		ui32Size;
	PSB_BOOL		bIsContiguous;

	/* If the buffer is contiguous we need only the physical 
	   start address. Otherwise we need the physical address 
	   of each page that makes up the buffer. */
	union
	{
		IMG_SYS_PHYADDR	sCont;
		IMG_SYS_PHYADDR	*psNonCont;
	} uSysAddr;

	IMG_DEV_VIRTADDR	sDevVAddr;
	IMG_CPU_VIRTADDR	pvCPUVAddr;

	PVRSRV_SYNC_DATA	*psSyncData;
} PVRPSB_BUFFER;

typedef struct PVRPSB_CURSOR_INFO_TAG
{
	PVRPSB_BUFFER	*psBuffer;

	IMG_UINT32	ui32Width;
	IMG_UINT32	ui32Height;

	IMG_INT32	i32X;
	IMG_INT32	i32Y;

	IMG_UINT32	ui32Mode;
	IMG_BOOL	bLoaded;
} PVRPSB_CURSOR_INFO;

/* Flip item structure used for queuing of flips */
typedef struct PVRPSB_VSYNC_FLIP_ITEM_TAG
{
	/* Command complete cookie to be passed to services 
	   command complete callback function */
	PSB_HANDLE	hCmdComplete;

	PVRPSB_BUFFER	*psBuffer;

	/* Number of frames for which this item should be displayed */
	IMG_UINT32	ui32SwapInterval;

	PSB_BOOL	bValid;

	PSB_BOOL	bFlipped;

	PSB_BOOL	bCmdCompleted;
} PVRPSB_VSYNC_FLIP_ITEM;

/* PVRPSB buffer structure */
typedef struct PVRPSB_SWAPCHAIN_TAG
{
	IMG_UINT32		ui32ID;

	IMG_UINT32		ui32BufferCount;
	PVRPSB_BUFFER		*psBuffer;

	/* Set of vsync flip items - enough for 1 
	   outstanding flip per buffer */
	PVRPSB_VSYNC_FLIP_ITEM	*psVSyncFlipItems;

	/* Insert index for the internal queue of flip items */
	IMG_UINT32		ui32InsertIndex;

	/* Remove index for the internal queue of flip items */
	IMG_UINT32		ui32RemoveIndex;
} PVRPSB_SWAPCHAIN;

typedef struct PVRPSB_GTT_INFO_TAG
{
	IMG_SYS_PHYADDR		sGTTSysAddr;
	IMG_CPU_VIRTADDR	pvGTTCPUVAddr;
	IMG_UINT32		ui32GTTSize;
	IMG_UINT32		ui32GTTOffset;

	IMG_DEV_VIRTADDR	sGMemDevVAddr;
	IMG_UINT32		ui32GMemSizeInPages;

	IMG_SYS_PHYADDR		sStolenSysAddr;
	IMG_UINT32		ui32StolenSizeInPages;
	IMG_UINT32		ui32StolenPageOffset;
} PVRPSB_GTT_INFO;

typedef struct PLL_FREQ_TAG
{
	IMG_UINT32 ui32Clock;
	IMG_UINT32 ui32Vco;
	IMG_UINT32 ui32M1;
	IMG_UINT32 ui32M2;
	IMG_UINT32 ui32N;
	IMG_UINT32 ui32P1;
	IMG_UINT32 ui32P2;
} PLL_FREQ;

/* Kernel device information structure */
typedef struct PVRPSB_DEVINFO_TAG
{
	PVRPSB_DEVICE			eDevice;

	/* Device ID assigned by services */
	IMG_UINT32			ui32ID;

	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;
	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;

	PVRPSB_GTT_INFO			sGTTInfo;

	IMG_SYS_PHYADDR			sRegSysAddr;
	IMG_CPU_VIRTADDR		pvRegCPUVAddr;

	DISPLAY_INFO			sDisplayInfo;
	DISPLAY_FORMAT			sDisplayFormat;
	DISPLAY_DIMS			sDisplayDims;

	/* List of supported display formats */
	IMG_UINT32			ui32NumFormats;
	DISPLAY_FORMAT			asDisplayFormatList[PVRPSB_MAX_FORMATS];

	/* List of supported display dimensions */
	IMG_UINT32			ui32NumDims;
	DISPLAY_DIMS			asDisplayDimList[PVRPSB_MAX_DIMS];

	PVRPSB_BUFFER			*psSystemBuffer;
	PVRPSB_BUFFER			*psCurrentBuffer;
#if defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
	PVRPSB_BUFFER			*psSavedBuffer;

	PSB_BOOL			bLeaveVT;
#endif

	PVRPSB_PIPE			eActivePipe;

	/* Back buffer info */
	IMG_UINT32			ui32TotalBackBuffers;
	PVRPSB_BUFFER			*apsBackBuffers[PVRPSB_MAX_BACKBUFFERS];

	/* Only one swapchain supported by this device so hang it here */
	PVRPSB_SWAPCHAIN		*psSwapChain;

	/* True if PVR is flushing its command queues */
	PSB_BOOL			bFlushCommands;

	PVRPSB_CURSOR_INFO		sCursorInfo;

#if defined(SUPPORT_DRI_DRM)
	struct drm_device		*psDrmDev;
	struct drm_fb_helper		sDrmFbHelper;

	PVRPSB_STATE			sSuspendState;

	PVRPSB_DEVICE_CAPS		sSdvoCapabilities;
#else
	struct pci_dev			*psPciDev;
#endif

	IMG_UINT32			ui32RefCount;
} PVRPSB_DEVINFO;

typedef struct PVRPSB_PLL_RANGE_TAG
{
	PVRPSB_DEVICE eDevice;

	IMG_UINT32 ui32DotClockMin;
	IMG_UINT32 ui32DotClockMax;

	IMG_UINT32 ui32RefFreq;

	IMG_UINT32 ui32NMin;
	IMG_UINT32 ui32NMax;

	IMG_UINT32 ui32MMin;
	IMG_UINT32 ui32MMax;
	IMG_UINT32 ui32M1Min;
	IMG_UINT32 ui32M1Max;
	IMG_UINT32 ui32M2Min;
	IMG_UINT32 ui32M2Max;

	IMG_UINT32 ui32PMin;
	IMG_UINT32 ui32PMax;
	IMG_UINT32 ui32P1Min;
	IMG_UINT32 ui32P1Max;
	IMG_UINT32 ui32P2Divide;
	IMG_UINT32 ui32P2Lo;
	IMG_UINT32 ui32P2Hi;

	IMG_UINT32 ui32VcoMin;
	IMG_UINT32 ui32VcoMax;
} PVRPSB_PLL_RANGE;


/******************************************************************************
 * Cedarview PLL ranges
 *****************************************************************************/
static const PVRPSB_PLL_RANGE sCdvSingleLvdsPllRange = 
{
	.eDevice		= PSB_CEDARVIEW,
	.ui32DotClockMin	= 20000,
	.ui32DotClockMax	= 115500,
	.ui32RefFreq		= 96000,
	.ui32NMin		= 2,
	.ui32NMax		= 6,
	.ui32MMin		= 60,
	.ui32MMax		= 160,
	.ui32M1Min		= 0,
	.ui32M1Max		= 0,
	.ui32M2Min		= 58,
	.ui32M2Max		= 158,
	.ui32PMin		= 28,
	.ui32PMax		= 140,
	.ui32P1Min		= 2,
	.ui32P1Max		= 10,
	.ui32P2Divide		= 200000,
	.ui32P2Lo		= 14,
	.ui32P2Hi		= 14,
	.ui32VcoMin		= 1800000,
	.ui32VcoMax		= 3600000,
};

static const PVRPSB_PLL_RANGE sCdvNonLvds27PllRange = 
{
	.eDevice		= PSB_CEDARVIEW,
	.ui32DotClockMin	= 20000,
	.ui32DotClockMax	= 400000,
	.ui32RefFreq		= 27000,
	.ui32NMin		= 1,
	.ui32NMax		= 1,
	.ui32MMin		= 67,
	.ui32MMax		= 132,
	.ui32M1Min		= 0,
	.ui32M1Max		= 0,
	.ui32M2Min		= 65,
	.ui32M2Max		= 130,
	.ui32PMin		= 5,
	.ui32PMax		= 90,
	.ui32P1Min		= 1,
	.ui32P1Max		= 9,
	.ui32P2Divide		= 225000,
	.ui32P2Lo		= 5,
	.ui32P2Hi		= 10,
	.ui32VcoMin		= 1809000,
	.ui32VcoMax		= 3564000,
};

static const PVRPSB_PLL_RANGE sCdvNonLvds96PllRange = 
{
	.eDevice		= PSB_CEDARVIEW,
	.ui32DotClockMin	= 20000,
	.ui32DotClockMax	= 400000,
	.ui32RefFreq		= 96000,
	.ui32NMin		= 2,
	.ui32NMax		= 6,
	.ui32MMin		= 60,
	.ui32MMax		= 160,
	.ui32M1Min		= 0,
	.ui32M1Max		= 0,
	.ui32M2Min		= 58,
	.ui32M2Max		= 158,
	.ui32PMin		= 5,
	.ui32PMax		= 100,
	.ui32P1Min		= 1,
	.ui32P1Max		= 10,
	.ui32P2Divide		= 225000,
	.ui32P2Lo		= 5,
	.ui32P2Hi		= 10,
	.ui32VcoMin		= 1800000,
	.ui32VcoMax		= 3600000,
};


/******************************************************************************
 * Poulsbo PLL ranges
 *****************************************************************************/
static const PVRPSB_PLL_RANGE sPsbSingleLvdsPllRange = 
{
	.eDevice		= PSB_POULSBO,
	.ui32DotClockMin	= 20000,
	.ui32DotClockMax	= 112000,
	.ui32RefFreq		= 96000,
	.ui32NMin		= 2,		/* The documentation says this value should be 3 but this can give the wrong vrefresh */
	.ui32NMax		= 8,
	.ui32MMin		= 70,
	.ui32MMax		= 120,
	.ui32M1Min		= 10,
	.ui32M1Max		= 20,
	.ui32M2Min		= 5,
	.ui32M2Max		= 9,
	.ui32PMin		= 7,
	.ui32PMax		= 98,
	.ui32P1Min		= 1,
	.ui32P1Max		= 8,
	.ui32P2Divide		= 112000,
	.ui32P2Lo		= 7,
	.ui32P2Hi		= 14,
	.ui32VcoMin		= 1400000,
	.ui32VcoMax		= 2800000,
};

static const PVRPSB_PLL_RANGE sPsbNonLvdsPllRange = 
{
	.eDevice		= PSB_POULSBO,
	.ui32DotClockMin	= 100000,
	.ui32DotClockMax	= 270000,
	.ui32RefFreq		= 96000,
	.ui32NMin		= 2,		/* The documentation says this value should be 3 but this can give the wrong vrefresh */
	.ui32NMax		= 8,
	.ui32MMin		= 70,
	.ui32MMax		= 120,
	.ui32M1Min		= 10,
	.ui32M1Max		= 20,
	.ui32M2Min		= 5,
	.ui32M2Max		= 9,
	.ui32PMin		= 5,
	.ui32PMax		= 80,
	.ui32P1Min		= 1,
	.ui32P1Max		= 8,
	.ui32P2Divide		= 200000,
	.ui32P2Lo		= 5,
	.ui32P2Hi		= 10,
	.ui32VcoMin		= 1400000,
	.ui32VcoMax		= 2800000,
};


/*******************************************************************************
 * OS independent functions
 ******************************************************************************/
PSB_ERROR PVRPSBInit(PVRPSB_DEVINFO *psDevInfo);
PSB_ERROR PVRPSBDeinit(PVRPSB_DEVINFO *psDevInfo);

PVRPSB_DEVINFO *PVRPSBGetDevInfo(IMG_VOID);
IMG_VOID PVRPSBSetDevInfo(PVRPSB_DEVINFO *psDevInfo);

PVRPSB_BUFFER *PVRPSBCreateBuffer(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32BufferSize);
void PVRPSBDestroyBuffer(PVRPSB_BUFFER *psBuffer);

IMG_UINT32 PVRPSBGetBpp(PVRSRV_PIXEL_FORMAT ePixelFormat);

PSB_BOOL PVRPSBSelectPLLFreq(IMG_UINT32 ui32DotClock, const PVRPSB_PLL_RANGE *psPllRange, PLL_FREQ *psPllFreq);

PSB_ERROR PVRPSBFlip(PVRPSB_DEVINFO *psDevInfo, PVRPSB_BUFFER *psBuffer);
void PVRPSBFlushInternalVSyncQueue(PVRPSB_DEVINFO *psDevInfo);

#if defined(SYS_USING_INTERRUPTS)
void PVRPSBEnableVSyncInterrupt(PVRPSB_DEVINFO *psDevInfo);
void PVRPSBDisableVSyncInterrupt(PVRPSB_DEVINFO *psDevInfo);
#endif /* #if defined(SYS_USING_INTERRUPTS) */

#if defined(POULSBO_DEVICE_ISR)
/* VSync ISR Functionality */
PSB_ERROR PVRPSBInstallVsyncISR(PVRPSB_DEVINFO *psDevInfo);
PSB_ERROR PVRPSBUninstallVsyncISR(PVRPSB_DEVINFO *psDevInfo);
#endif /* #if defined(POULSBO_DEVICE_ISR) */


/*******************************************************************************
 * OS specific functions
 ******************************************************************************/
PSB_ERROR PVROSModeSetInit(PVRPSB_DEVINFO *psDevInfo);
IMG_VOID PVROSModeSetDeinit(PVRPSB_DEVINFO *psDevInfo);

void PVROSSaveState(PVRPSB_DEVINFO *psDevInfo);
void PVROSRestoreState(PVRPSB_DEVINFO *psDevInfo);

void PVROSDelayus(IMG_UINT32 ui32Timeus);
void PVROSSleepms(IMG_UINT32 ui32Timems);

void *PVROSAllocKernelMem(unsigned long ulSize);
void *PVROSCallocKernelMem(unsigned long ulSize);
void PVROSFreeKernelMem(void *pvMem);

IMG_CPU_VIRTADDR PVROSAllocKernelMemForBuffer(unsigned long ulSize, IMG_SYS_PHYADDR *psSysAddr);
IMG_VOID PVROSFreeKernelMemForBuffer(IMG_CPU_VIRTADDR pvCPUVAddr);

/* Returns a CPU-Virtual address for the Sys-Phys address specified, 
   with the specified range of addresses mapped. */
void *PVROSMapPhysAddr(IMG_SYS_PHYADDR sSysAddr, IMG_UINT32 ui32Size);
void *PVROSMapPhysAddrWC(IMG_SYS_PHYADDR sSysAddr, IMG_UINT32 ui32Size);

/* Unmap a virtual address range previously returned by MapPhysAddr */
void PVROSUnMapPhysAddr(void *pvAddr);

/* Read/Write DWords in GMA500 PCI Configuration Space - see Ch9 of
   http://download.intel.com/design/chipsets/embedded/datashts/319537.pdf and
   http://download.intel.com/design/chipsets/embedded/datashts/321422.pdf */
IMG_UINT32 PVROSPciReadDWord(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32Reg);
IMG_VOID PVROSPciWriteDWord(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32Reg, IMG_UINT32 ui32Value);

IMG_UINT16 PVROSPciReadWord(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32Reg);
IMG_VOID PVROSPciWriteWord(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32Reg, IMG_UINT16 ui32Value);

IMG_UINT8 PVROSPciReadByte(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32Reg);
IMG_VOID PVROSPciWriteByte(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32Reg, IMG_UINT8 ui32Value);

IMG_UINT32 PVROSReadIOMem(void *pvRegAddr);
void PVROSWriteIOMem(void *pvRegAddr, IMG_UINT32 ui32Value);

/*Read and Write to Memory-Mapped Display Registers (the set of registers
  from Ch2 of http://intellinuxgraphics.org/VOL_3_display_registers_updated.pdf)
  and also in Ch8 of http://intellinuxgraphics.org/VOL_1_graphics_core.pdf) */
#define PVROSReadMMIOReg(psDevInfo, ui32RegOffset) \
	PVROSReadIOMem((psDevInfo)->pvRegCPUVAddr + ui32RegOffset)

#define PVROSWriteMMIOReg(psDevInfo, ui32RegOffset, ui32Value) \
	PVROSWriteIOMem((psDevInfo)->pvRegCPUVAddr + ui32RegOffset, ui32Value)

void PVROSSetIOMem(void *pvAddr, IMG_UINT8 ui8Value, IMG_UINT32 ui32Size);

void PVROSCopyToIOMem(void *pvDstAddr, void *pvSrcAddr, IMG_UINT32 ui32Size);
void PVROSCopyFromIOMem(void *pvDstAddr, void *pvSrcAddr, IMG_UINT32 ui32Size);

#if defined(__cplusplus)
}
#endif

#endif /* #if !defined(__DC_POULSBO_H__) */

/******************************************************************************
 End of file (dc_poulsbo.h)
******************************************************************************/

