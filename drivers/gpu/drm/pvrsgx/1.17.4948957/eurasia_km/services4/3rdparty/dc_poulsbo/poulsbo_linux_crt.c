/*************************************************************************/ /*!
@Title          Poulsbo linux-specific CRT functions.
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

#include "poulsbo_linux.h"
#include <linux/delay.h>

#if defined(SUPPORT_DRI_DRM)
#define PVRPSB_HOTPLUG_RETRIES (100)
#endif

#define WAIT_DPIO_RDY_RETURN(devInfo, retryCount, returnVal)		\
	do								\
	{								\
		IMG_UINT32 ui32Retries = retryCount;			\
		while (PVRPSB_DPIO_CMD_BUSY_GET(PVROSReadMMIOReg(devInfo, PVRPSB_DPIO_CMD))) \
		{							\
			msleep(1);					\
			if (--ui32Retries == 0)				\
			{						\
				return returnVal;			\
			}						\
		}							\
	} while(0)

static PSB_BOOL ReadDPIOReg(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32Reg, IMG_UINT32 *pui32Value)
{
	IMG_UINT32 ui32RegVal;

	/* Make sure there is no DPIO operation already in progress */
	WAIT_DPIO_RDY_RETURN(psDevInfo, 1000, PSB_FALSE);

	/* Set the address of the register we want to read */
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_DPIO_ADDR, ui32Reg);

	/* Issue the read command */
	ui32RegVal = 0;
	ui32RegVal = PVRPSB_DPIO_CMD_OP_SET(ui32RegVal, PVRPSB_DPIO_CMD_OP_READ);
	ui32RegVal = PVRPSB_DPIO_CMD_TARGET_SET(ui32RegVal, PVRPSB_DPIO_CMD_TARGET_DPLL);
	ui32RegVal = PVRPSB_DPIO_CMD_ENABLE_SET(ui32RegVal, 0xF);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_DPIO_CMD, ui32RegVal);

	/* Wait for the read to complete */
	WAIT_DPIO_RDY_RETURN(psDevInfo, 1000, PSB_FALSE);

	/* Get the value that was read from the register */
	*pui32Value = PVROSReadMMIOReg(psDevInfo, PVRPSB_DPIO_DATA);

	return PSB_TRUE;
}

static PSB_BOOL WriteDPIOReg(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32Reg, IMG_UINT32 ui32Value)
{
	IMG_UINT32 ui32RegVal;

	/* Make sure there is no DPIO operation already in progress */
	WAIT_DPIO_RDY_RETURN(psDevInfo, 1000, PSB_FALSE);

	/* Set the address of the register to which we want to write and the data that should be written */
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_DPIO_ADDR, ui32Reg);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_DPIO_DATA, ui32Value);

	/* Issue the write command */
	ui32RegVal = 0;
	ui32RegVal = PVRPSB_DPIO_CMD_OP_SET(ui32RegVal, PVRPSB_DPIO_CMD_OP_WRITE);
	ui32RegVal = PVRPSB_DPIO_CMD_TARGET_SET(ui32RegVal, PVRPSB_DPIO_CMD_TARGET_DPLL);
	ui32RegVal = PVRPSB_DPIO_CMD_ENABLE_SET(ui32RegVal, 0xF);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_DPIO_CMD, ui32RegVal);

	/* Wait for the write to complete */
	WAIT_DPIO_RDY_RETURN(psDevInfo, 1000, PSB_FALSE);

	return PSB_TRUE;
}

#define RETURN_ON_FALSE(returnVal) if (returnVal == PSB_FALSE) return

#if defined(SUPPORT_DRI_DRM)
static void CRTProgramPLL(PVRPSB_DEVINFO *psDevInfo, PLL_FREQ *psPllFreqInfo, PVRPSB_PIPE ePipe)
#else
void CRTProgramPLL(PVRPSB_DEVINFO *psDevInfo, PLL_FREQ *psPllFreqInfo, PVRPSB_PIPE ePipe)
#endif
{
	const IMG_UINT32 ui32DpioRefReg		= (ePipe == PSB_PIPE_A) ? PVRPSB_DPIO_ADDR_REF_A : PVRPSB_DPIO_ADDR_REF_B;
	const IMG_UINT32 ui32DpioMReg		= (ePipe == PSB_PIPE_A) ? PVRPSB_DPIO_ADDR_M_A : PVRPSB_DPIO_ADDR_M_B;
	const IMG_UINT32 ui32DpioNVcoReg	= (ePipe == PSB_PIPE_A) ? PVRPSB_DPIO_ADDR_N_VCO_A : PVRPSB_DPIO_ADDR_N_VCO_B;
	const IMG_UINT32 ui32DpioPReg		= (ePipe == PSB_PIPE_A) ? PVRPSB_DPIO_ADDR_P_A : PVRPSB_DPIO_ADDR_P_B;
	IMG_UINT32 ui32RegVal;
	IMG_UINT32 ui32NVco;
	IMG_UINT32 ui32M;
	IMG_UINT32 ui32P;
	IMG_UINT32 ui32Lane;

	/* Reset the DPIO */
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_DPIO_CFG, 0);
	PVROSReadMMIOReg(psDevInfo, PVRPSB_DPIO_CFG);

	ui32RegVal = 0;
	ui32RegVal = PVRPSB_DPIO_CFG_MODE_SET(ui32RegVal, PVRPSB_DPIO_CFG_MODE_SELECT0);
	ui32RegVal = PVRPSB_DPIO_CFG_RESET_SET(ui32RegVal, 1);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_DPIO_CFG, ui32RegVal);

	/* Program the PLL registers */
	RETURN_ON_FALSE(WriteDPIOReg(psDevInfo, ui32DpioRefReg, 0x0068A701)); /* Some magic */

	RETURN_ON_FALSE(ReadDPIOReg(psDevInfo, ui32DpioMReg, &ui32M));
	ui32M = PVRPSB_DPIO_DATA_M_M2_SET(ui32M, psPllFreqInfo->ui32M2);
	RETURN_ON_FALSE(WriteDPIOReg(psDevInfo, ui32DpioMReg, ui32M));

	RETURN_ON_FALSE(ReadDPIOReg(psDevInfo, ui32DpioNVcoReg, &ui32NVco));
	ui32NVco = PVRPSB_DPIO_DATA_N_VCO_N_SET(ui32NVco, psPllFreqInfo->ui32N);
	ui32NVco = PVRPSB_DPIO_DATA_N_VCO_MAGIC_SET(ui32NVco, 0x0107); /* Some more magic for good measure */

	if (psPllFreqInfo->ui32Vco < 2250000)
	{
		ui32NVco = PVRPSB_DPIO_DATA_N_VCO_SEL_SET(ui32NVco, 0);
		ui32NVco = PVRPSB_DPIO_DATA_N_VCO_CB_TUNE_SET(ui32NVco, 2);
	}
	else if (psPllFreqInfo->ui32Vco < 2750000)
	{
		ui32NVco = PVRPSB_DPIO_DATA_N_VCO_SEL_SET(ui32NVco, 1);
		ui32NVco = PVRPSB_DPIO_DATA_N_VCO_CB_TUNE_SET(ui32NVco, 1);
	}
	else if (psPllFreqInfo->ui32Vco < 3300000)
	{
		ui32NVco = PVRPSB_DPIO_DATA_N_VCO_SEL_SET(ui32NVco, 2);
		ui32NVco = PVRPSB_DPIO_DATA_N_VCO_CB_TUNE_SET(ui32NVco, 0);
	}
	else
	{
		ui32NVco = PVRPSB_DPIO_DATA_N_VCO_SEL_SET(ui32NVco, 3);
		ui32NVco = PVRPSB_DPIO_DATA_N_VCO_CB_TUNE_SET(ui32NVco, 0);
	}
	RETURN_ON_FALSE(WriteDPIOReg(psDevInfo, ui32DpioNVcoReg, ui32NVco));

	RETURN_ON_FALSE(ReadDPIOReg(psDevInfo, ui32DpioPReg, &ui32P));
	ui32P = PVRPSB_DPIO_DATA_P_P1_SET(ui32P, psPllFreqInfo->ui32P1);

	if (psPllFreqInfo->ui32P2 == 10)
	{
		ui32P = PVRPSB_DPIO_DATA_P_P2_DIVIDE_SET(ui32P, PVRPSB_DPIO_DATA_P_P2_DIVIDE_10);
	}
	else if (psPllFreqInfo->ui32P2 == 5)
	{
		ui32P = PVRPSB_DPIO_DATA_P_P2_DIVIDE_SET(ui32P, PVRPSB_DPIO_DATA_P_P2_DIVIDE_5);
	}
	else
	{
		printk(KERN_WARNING DRVNAME " - %s: Unrecognised P2 value\n", __FUNCTION__);
	}
	RETURN_ON_FALSE(WriteDPIOReg(psDevInfo, ui32DpioPReg, ui32P));

	if (ePipe == PSB_PIPE_A)
	{
		RETURN_ON_FALSE(ReadDPIOReg(psDevInfo, PVRPSB_DPIO_ADDR_LANE0, &ui32Lane));
		ui32Lane = PVRPSB_DPIO_DATA_LANE_ENABLE_SET(ui32Lane, 3);
		RETURN_ON_FALSE(WriteDPIOReg(psDevInfo, PVRPSB_DPIO_ADDR_LANE0, ui32Lane));

		RETURN_ON_FALSE(ReadDPIOReg(psDevInfo, PVRPSB_DPIO_ADDR_LANE1, &ui32Lane));
		ui32Lane = PVRPSB_DPIO_DATA_LANE_ENABLE_SET(ui32Lane, 3);
		RETURN_ON_FALSE(WriteDPIOReg(psDevInfo, PVRPSB_DPIO_ADDR_LANE1, ui32Lane));

		RETURN_ON_FALSE(ReadDPIOReg(psDevInfo, PVRPSB_DPIO_ADDR_LANE2, &ui32Lane));
		ui32Lane = PVRPSB_DPIO_DATA_LANE_ENABLE_SET(ui32Lane, 3);
		RETURN_ON_FALSE(WriteDPIOReg(psDevInfo, PVRPSB_DPIO_ADDR_LANE2, ui32Lane));

		RETURN_ON_FALSE(ReadDPIOReg(psDevInfo, PVRPSB_DPIO_ADDR_LANE3, &ui32Lane));
		ui32Lane = PVRPSB_DPIO_DATA_LANE_ENABLE_SET(ui32Lane, 3);
		RETURN_ON_FALSE(WriteDPIOReg(psDevInfo, PVRPSB_DPIO_ADDR_LANE3, ui32Lane));
	}
}

#if defined(SUPPORT_DRI_DRM)
/******************************************************************************
 * Connector functions
 ******************************************************************************/

static int CRTConnectorHelperGetModes(struct drm_connector *psConnector)
{
	PVRPSB_CONNECTOR *psPVRConnector	= to_pvr_connector(psConnector);
	struct i2c_adapter *psAdapter		= psPVRConnector->psAdapter;
	struct edid *psEdid;

	psEdid = drm_get_edid(psConnector, psAdapter);
	if (psEdid == NULL)
	{
		return 0;
	}

	drm_mode_connector_update_edid_property(psConnector, psEdid);

	return drm_add_edid_modes(psConnector, psEdid);
}

static int CRTConnectorHelperModeValid(struct drm_connector *psConnector, struct drm_display_mode *psMode)
{
	PVRPSB_DEVINFO *psDevInfo		= (PVRPSB_DEVINFO *)psConnector->dev->dev_private;
	const PVRPSB_PLL_RANGE *psPllRange	= NULL;
	int iModeStatus				= MODE_BAD;

	switch (psDevInfo->eDevice)
	{
		case PSB_CEDARVIEW:
			psPllRange = &sCdvNonLvds27PllRange;
			break;
		case PSB_POULSBO:
			psPllRange = &sPsbNonLvdsPllRange;
			break;
		case PSB_UNKNOWN:
			return iModeStatus;
	}

	if (psMode->flags & DRM_MODE_FLAG_INTERLACE)
	{
		iModeStatus = MODE_NO_INTERLACE;
	}
	else if (psMode->flags & DRM_MODE_FLAG_DBLSCAN)
	{
		iModeStatus = MODE_NO_DBLESCAN;
	}
	else if (psMode->clock < psPllRange->ui32DotClockMin)
	{
		iModeStatus = MODE_CLOCK_LOW;
	}
	else if (psMode->clock > psPllRange->ui32DotClockMax)
	{
		iModeStatus = MODE_CLOCK_HIGH;
	}
#if defined(PVRPSB_WIDTH) && defined(PVRPSB_HEIGHT)
	else if (drm_mode_width(psMode) != PVRPSB_WIDTH || drm_mode_height(psMode) != PVRPSB_HEIGHT)
	{
		iModeStatus = MODE_ONE_SIZE;
	}
#endif
#if defined(PVRPSB_VREFRESH)
	else if (drm_mode_vrefresh(psMode) != PVRPSB_VREFRESH)
	{
		iModeStatus = MODE_VSYNC;
	}
#endif
	else
	{
		iModeStatus = MODE_OK;
	}

	return iModeStatus;
}

static struct drm_encoder *CRTConnectorHelperBestEncoder(struct drm_connector *psConnector)
{
	struct drm_mode_object *psObject;

	/* Pick the first encoder we find */
	if (psConnector->encoder_ids[0] != 0)
	{
		psObject = drm_mode_object_find(psConnector->dev, psConnector->encoder_ids[0], DRM_MODE_OBJECT_ENCODER);
		if (psObject != NULL)
		{
			return obj_to_encoder(psObject);
		}		
	}

	return NULL;
}

static struct drm_connector_helper_funcs sCRTConnectorHelperFuncs = 
{
	.get_modes	= CRTConnectorHelperGetModes, 
	.mode_valid	= CRTConnectorHelperModeValid, 
	.best_encoder	= CRTConnectorHelperBestEncoder, 
};

static void CRTConnectorSave(struct drm_connector *psConnector)
{
}

static void CRTConnectorRestore(struct drm_connector *psConnector)
{
}

static enum drm_connector_status CRTConnectorDetect(struct drm_connector *psConnector, bool force)
{
	PVRPSB_DEVINFO *psDevInfo		= (PVRPSB_DEVINFO *)psConnector->dev->dev_private;
	enum drm_connector_status eStatus	= connector_status_disconnected;
	IMG_UINT32 ui32Retries;
	IMG_UINT32 ui32AdpaCtlVal;
	IMG_UINT32 ui32HotPlugVal;

	/* Store out the ADPA control register value and turn off the ADPA output before using the hotplug registers */
	ui32AdpaCtlVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_ADPA_CTL);
	if (PVRPSB_ADPA_CTL_ENABLE_GET(ui32AdpaCtlVal) != 0)
	{
		PVROSWriteMMIOReg(psDevInfo, PVRPSB_ADPA_CTL, 0);
	}

	/* Clear the hot plug status register */
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_HOTPLUG_STAT, 0);

	/* Force a hotplug/unplug detection cycle */
	ui32HotPlugVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_HOTPLUG);
	ui32HotPlugVal = PVRPSB_HOTPLUG_CRT_EN_SET(ui32HotPlugVal, 1);
	ui32HotPlugVal = PVRPSB_HOTPLUG_CRT_PERIOD_SET(ui32HotPlugVal, PVRPSB_HOTPLUG_CRT_PERIOD_64);
	ui32HotPlugVal = PVRPSB_HOTPLUG_CRT_VOLT_SET(ui32HotPlugVal, PVRPSB_HOTPLUG_CRT_VOLT_50);
	ui32HotPlugVal = PVRPSB_HOTPLUG_CRT_TRIGGER_SET(ui32HotPlugVal, 1);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_HOTPLUG, ui32HotPlugVal);

	/* It seems that the hot plug status register doesn't report that an interrupt has 
	   occurred so check the CRT hot plug trigger bit has been unset instead. */
	for (ui32Retries = PVRPSB_HOTPLUG_RETRIES; PVRPSB_HOTPLUG_CRT_TRIGGER_GET(PVROSReadMMIOReg(psDevInfo, PVRPSB_HOTPLUG)) && ui32Retries != 0; ui32Retries--)
	{
		msleep(1);
	}

	if (psDevInfo->eDevice == PSB_CEDARVIEW)
	{
		/* Do it again for luck */
		PVROSWriteMMIOReg(psDevInfo, PVRPSB_HOTPLUG, ui32HotPlugVal);
		for (ui32Retries = PVRPSB_HOTPLUG_RETRIES; PVRPSB_HOTPLUG_CRT_EN_GET(PVROSReadMMIOReg(psDevInfo, PVRPSB_HOTPLUG)) && ui32Retries != 0; ui32Retries--)
		{
			msleep(1);
		}
	}

	if (PVRPSB_HOTPLUG_CRT_DET_STAT_GET(PVROSReadMMIOReg(psDevInfo, PVRPSB_HOTPLUG_STAT)) != PVRPSB_HOTPLUG_CRT_DET_STAT_NONE)
	{
		eStatus = connector_status_connected;
	}

	/* Disable CRT hot plug */
	ui32HotPlugVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_HOTPLUG);
	ui32HotPlugVal = PVRPSB_HOTPLUG_CRT_EN_SET(ui32HotPlugVal, 0);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_HOTPLUG, ui32HotPlugVal);

	/* Write back the original ADPA_CTL value */
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_ADPA_CTL, ui32AdpaCtlVal);

	return eStatus;
}

static int CRTConnectorSetProperty(struct drm_connector *psConnector, struct drm_property *psProperty, uint64_t ui64Val)
{
	return -ENOSYS;
}

static void CRTConnectorDestroy(struct drm_connector *psConnector)
{
	PVRPSB_CONNECTOR *psPVRConnector = to_pvr_connector(psConnector);

	drm_mode_connector_update_edid_property(psConnector, NULL);
	drm_connector_cleanup(psConnector);

	PVRI2CAdapterDestroy(psPVRConnector->psAdapter);

	PVROSFreeKernelMem(psPVRConnector);
}

static void CRTConnectorForce(struct drm_connector *psConnector)
{
	/* Not supported */
}

static const struct drm_connector_funcs sCRTConnectorFuncs = 
{
	.dpms		= drm_helper_connector_dpms, 
	.save		= CRTConnectorSave, 
	.restore	= CRTConnectorRestore, 
	.detect		= CRTConnectorDetect, 
	.fill_modes	= drm_helper_probe_single_connector_modes, 
	.set_property	= CRTConnectorSetProperty, 
	.destroy	= CRTConnectorDestroy, 
	.force		= CRTConnectorForce, 
};

static PVRPSB_CONNECTOR *CRTConnectorCreate(PVRPSB_DEVINFO *psDevInfo, struct i2c_adapter *psAdapter)
{
	PVRPSB_CONNECTOR *psPVRConnector;

	psPVRConnector = (PVRPSB_CONNECTOR *)kzalloc(sizeof(PVRPSB_CONNECTOR), GFP_KERNEL);
	if (psPVRConnector)
	{
		drm_connector_init(psDevInfo->psDrmDev, &psPVRConnector->sConnector, &sCRTConnectorFuncs, DRM_MODE_CONNECTOR_VGA);
		drm_connector_helper_add(&psPVRConnector->sConnector, &sCRTConnectorHelperFuncs);

		psPVRConnector->psAdapter	= psAdapter;
		psPVRConnector->ePort		= PSB_PORT_ANALOG;
	}

	return psPVRConnector;
}

/******************************************************************************
 * Encoder functions
 ******************************************************************************/

static void CRTEncoderHelperDpms(struct drm_encoder *psEncoder, int iMode)
{
	PVRPSB_DEVINFO *psDevInfo = (PVRPSB_DEVINFO *)psEncoder->dev->dev_private;
	IMG_UINT32 ui32RegVal;

	ui32RegVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_ADPA_CTL);

	switch (iMode)
	{
		case DRM_MODE_DPMS_ON:
		{
			ui32RegVal = PVRPSB_ADPA_CTL_DPMS_SET(ui32RegVal, PVRPSB_ADPA_CTL_DPMS_ON);
			break;
		}
		case DRM_MODE_DPMS_STANDBY:
		{
			ui32RegVal = PVRPSB_ADPA_CTL_DPMS_SET(ui32RegVal, PVRPSB_ADPA_CTL_DPMS_STANDBY);
			break;
		}
		case DRM_MODE_DPMS_SUSPEND:
		{
			ui32RegVal = PVRPSB_ADPA_CTL_DPMS_SET(ui32RegVal, PVRPSB_ADPA_CTL_DPMS_SUSPEND);
			break;
		}
		case DRM_MODE_DPMS_OFF:
		{
			ui32RegVal = PVRPSB_ADPA_CTL_DPMS_SET(ui32RegVal, PVRPSB_ADPA_CTL_DPMS_OFF);
			break;
		}
	}

	PVROSWriteMMIOReg(psDevInfo, PVRPSB_ADPA_CTL, ui32RegVal);
}

static bool CRTEncoderHelperModeFixup(struct drm_encoder *psEncoder, const struct drm_display_mode *psMode, struct drm_display_mode *psAdjustedMode)
{
	/* Nothing to do */
	return true;
}

static void CRTEncoderHelperPrepare(struct drm_encoder *psEncoder)
{
	PVRPSB_DEVINFO *psDevInfo	= (PVRPSB_DEVINFO *)psEncoder->dev->dev_private;
	PVRPSB_CRTC *psPVRCrtc		= to_pvr_crtc(psEncoder->crtc);
	IMG_UINT32 ui32RegVal;

	/* Disable port stall */
	ui32RegVal = 0;
	ui32RegVal = PVRPSB_ADPA_CTL_ENABLE_SET(ui32RegVal, 1);
	ui32RegVal = PVRPSB_ADPA_CTL_PIPE_SET(ui32RegVal, psPVRCrtc->ePipe);
	ui32RegVal = PVRPSB_ADPA_CTL_DPMS_SET(ui32RegVal, PVRPSB_ADPA_CTL_DPMS_OFF);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_ADPA_CTL, ui32RegVal);

	/* Disable port */
	ui32RegVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_ADPA_CTL);
	ui32RegVal = PVRPSB_ADPA_CTL_ENABLE_SET(ui32RegVal, 0);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_ADPA_CTL, ui32RegVal);
}

static void CRTEncoderHelperCommit(struct drm_encoder *psEncoder)
{
	PVRPSB_DEVINFO *psDevInfo = (PVRPSB_DEVINFO *)psEncoder->dev->dev_private;
	IMG_UINT32 ui32RegVal;

	/* Enable port */
	ui32RegVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_ADPA_CTL);
	ui32RegVal = PVRPSB_ADPA_CTL_ENABLE_SET(ui32RegVal, 1);
	ui32RegVal = PVRPSB_ADPA_CTL_DPMS_SET(ui32RegVal, PVRPSB_ADPA_CTL_DPMS_ON);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_ADPA_CTL, ui32RegVal);
}

static void CRTEncoderHelperModeSet(struct drm_encoder *psEncoder, struct drm_display_mode *psMode, struct drm_display_mode *psAdjustedMode)
{
	PVRPSB_DEVINFO *psDevInfo		= (PVRPSB_DEVINFO *)psEncoder->dev->dev_private;
	PVRPSB_CRTC *psPVRCrtc			= to_pvr_crtc(psEncoder->crtc);
	const IMG_UINT32 ui32DpllCtlReg		= (psPVRCrtc->ePipe == PSB_PIPE_A) ? PVRPSB_DPLLA_CTL : PVRPSB_DPLLB_CTL;
	const IMG_UINT32 ui32DpllMd		= (psPVRCrtc->ePipe == PSB_PIPE_A) ? PVRPSB_DPLLAMD : PVRPSB_DPLLBMD;
	const PVRPSB_PLL_RANGE *psPllRange	= NULL;
	PLL_FREQ sPllFreqInfo;
	IMG_UINT32 ui32PllCtl;
	IMG_UINT32 ui32RegVal;

	switch (psDevInfo->eDevice)
	{
		case PSB_CEDARVIEW:
			psPllRange = &sCdvNonLvds27PllRange;
			break;
		case PSB_POULSBO:
			psPllRange = &sPsbNonLvdsPllRange;
			break;
		case PSB_UNKNOWN:
			printk(KERN_ERR DRVNAME " - %s: Unknown device\n", __FUNCTION__);
			return;
	}

	/* Determine PLL data for the given clock */
	if (PVRPSBSelectPLLFreq(psMode->clock, psPllRange, &sPllFreqInfo) == PSB_FALSE)
	{
		printk(KERN_ERR DRVNAME " - %s: PVRPSBSelectPLLFreq failed\n", __FUNCTION__);
		return;
	}

	/* Program PLL related registers */
	ui32PllCtl = 0;
	ui32PllCtl = PVRPSB_DPLL_ENABLE_SET(ui32PllCtl, 0);
	ui32PllCtl = PVRPSB_DPLL_HIGHSPEED_ENABLE_SET(ui32PllCtl, 0);
	ui32PllCtl = PVRPSB_DPLL_VGA_DISABLE_SET(ui32PllCtl, 1);
	ui32PllCtl = PVRPSB_DPLL_MODE_SET(ui32PllCtl, PVRPSB_DPLL_MODE_SDVO);
	ui32PllCtl = PVRPSB_DPLL_REF_CLOCK_SET(ui32PllCtl, PVRPSB_DPLL_REF_CLOCK_DEFAULT);
	ui32PllCtl = PVRPSB_DPLL_CLOCK_MULTI_SET(ui32PllCtl, 1);
	ui32PllCtl = PVRPSB_DPLL_P2_SET(ui32PllCtl, ((10 - sPllFreqInfo.ui32P2) / 5));

	if (sPllFreqInfo.ui32P2 == sCdvNonLvds27PllRange.ui32P2Hi)
	{
		ui32PllCtl = PVRPSB_DPLL_P2_DIVIDE_SET(ui32PllCtl, PVRPSB_DPLL_P2_DIVIDE_SDVO_10);
	}
	else
	{
		ui32PllCtl = PVRPSB_DPLL_P2_DIVIDE_SET(ui32PllCtl, PVRPSB_DPLL_P2_DIVIDE_SDVO_5);
	}

	/* Device specific setup of the DPLL register */
	switch (psDevInfo->eDevice)
	{
		case PSB_CEDARVIEW:
			ui32PllCtl = PVRPSB_DPLL_SYNC_CLOCK_ENABLE_SET(ui32PllCtl, 1);
			break;
		case PSB_POULSBO:
			ui32PllCtl = PVRPSB_DPLL_P1_POSTDIVIDE_SET(ui32PllCtl, (1 << (sPllFreqInfo.ui32P1 - 1)));
			break;
		case PSB_UNKNOWN:
			break;
	}

	PVROSWriteMMIOReg(psDevInfo, ui32DpllCtlReg, ui32PllCtl);
	PVROSDelayus(150);

	CRTProgramPLL(psDevInfo, &sPllFreqInfo, psPVRCrtc->ePipe);
	PVROSDelayus(150);

	ui32PllCtl = PVRPSB_DPLL_ENABLE_SET(ui32PllCtl, 1);
	PVROSWriteMMIOReg(psDevInfo, ui32DpllCtlReg, ui32PllCtl);
	ui32PllCtl = PVROSReadMMIOReg(psDevInfo, ui32DpllCtlReg);

	PVROSDelayus(150);

	PVROSWriteMMIOReg(psDevInfo, ui32DpllCtlReg, ui32PllCtl);
	PVROSReadMMIOReg(psDevInfo, ui32DpllCtlReg);
	PVROSDelayus(150);


	ui32RegVal = PVROSReadMMIOReg(psDevInfo, ui32DpllMd);
	ui32RegVal = PVRPSB_DPLLMD_DIV_HIRES_SET(ui32RegVal, 0);
	ui32RegVal = PVRPSB_DPLLMD_MUL_HIRES_SET(ui32RegVal, 0);
	PVROSWriteMMIOReg(psDevInfo, ui32DpllMd, ui32RegVal);


	ui32RegVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_ADPA_CTL);
	ui32RegVal = PVRPSB_ADPA_CTL_POLARITY_SEL_SET(ui32RegVal, PVRPSB_ADPA_CTL_POLARITY_SEL_ADPA);

	if (psAdjustedMode->flags & DRM_MODE_FLAG_PHSYNC)
	{
		ui32RegVal = PVRPSB_ADPA_CTL_HPOLARITY_CTL_SET(ui32RegVal, PVRPSB_ADPA_CTL_VPOLARITY_CTL_ACTIVE_HI);
	}
	else
	{
		ui32RegVal = PVRPSB_ADPA_CTL_HPOLARITY_CTL_SET(ui32RegVal, PVRPSB_ADPA_CTL_VPOLARITY_CTL_ACTIVE_LO);
	}

	if (psAdjustedMode->flags & DRM_MODE_FLAG_PVSYNC)
	{
		ui32RegVal = PVRPSB_ADPA_CTL_VPOLARITY_CTL_SET(ui32RegVal, PVRPSB_ADPA_CTL_VPOLARITY_CTL_ACTIVE_HI);
	}
	else
	{
		ui32RegVal = PVRPSB_ADPA_CTL_VPOLARITY_CTL_SET(ui32RegVal, PVRPSB_ADPA_CTL_VPOLARITY_CTL_ACTIVE_LO);
	}

	PVROSWriteMMIOReg(psDevInfo, PVRPSB_ADPA_CTL, ui32RegVal);
}

static const struct drm_encoder_helper_funcs sCRTEncoderHelperFuncs = 
{
	.dpms		= CRTEncoderHelperDpms, 
	.save		= NULL, 
	.restore	= NULL, 
	.mode_fixup	= CRTEncoderHelperModeFixup, 
	.prepare	= CRTEncoderHelperPrepare, 
	.commit		= CRTEncoderHelperCommit, 
	.mode_set	= CRTEncoderHelperModeSet, 
	.get_crtc	= NULL, 
	.detect		= NULL, 
	.disable	= NULL, 
};

static void CRTEncoderDestroy(struct drm_encoder *psEncoder)
{
	PVRPSB_ENCODER *psPVREncoder = to_pvr_encoder(psEncoder);

	drm_encoder_cleanup(psEncoder);
	PVROSFreeKernelMem(psPVREncoder);
}

static const struct drm_encoder_funcs sCRTEncoderFuncs = 
{
	.destroy = CRTEncoderDestroy, 
};

static PVRPSB_ENCODER *CRTEncoderCreate(PVRPSB_DEVINFO *psDevInfo)
{
	PVRPSB_ENCODER *psPVREncoder;

	psPVREncoder = (PVRPSB_ENCODER *)kzalloc(sizeof(PVRPSB_ENCODER), GFP_KERNEL);
	if (psPVREncoder)
	{
		drm_encoder_init(psDevInfo->psDrmDev, &psPVREncoder->sEncoder, &sCRTEncoderFuncs, DRM_MODE_ENCODER_DAC);
		drm_encoder_helper_add(&psPVREncoder->sEncoder, &sCRTEncoderHelperFuncs);

		/* This is a bit field that's used to determine by which CRTCs the encoder can be driven. 
		   We have only one CRTC so always set to 0x1 */
		psPVREncoder->sEncoder.possible_crtcs = 0x1;
	}

	return psPVREncoder;
}

/******************************************************************************
 * Non-static functions
 ******************************************************************************/
PSB_ERROR CRTSetup(PVRPSB_DEVINFO *psDevInfo)
{
	struct i2c_adapter *psAdapter;
	PVRPSB_CONNECTOR *psPVRConnector;
	PVRPSB_ENCODER *psPVREncoder;

	psAdapter = PVRI2CAdapterCreate(psDevInfo, "PSB I2C Adapter (CRT)", PVRPSB_GPIO_A, 0);
	if (psAdapter == NULL)
	{
		printk(KERN_ERR DRVNAME " - %s: failed to create an I2C adapter.\n", __FUNCTION__);

		return PSB_ERROR_OUT_OF_MEMORY;
	}

	psPVRConnector = CRTConnectorCreate(psDevInfo, psAdapter);
	if (psPVRConnector == NULL)
	{
		printk(KERN_ERR DRVNAME " - %s: failed to create an CRT connector.\n", __FUNCTION__);
				
		PVRI2CAdapterDestroy(psAdapter);
		return PSB_ERROR_OUT_OF_MEMORY;
	}

	psPVREncoder = CRTEncoderCreate(psDevInfo);
	if (psPVREncoder == NULL)
	{
		printk(KERN_ERR DRVNAME " - %s: failed to create a CRT encoder.\n", __FUNCTION__);

		return PSB_ERROR_OUT_OF_MEMORY;
	}
	drm_mode_connector_attach_encoder(&psPVRConnector->sConnector, &psPVREncoder->sEncoder);

	psDevInfo->sDisplayInfo.ui32PhysicalWidthmm	= psPVRConnector->sConnector.display_info.width_mm;
	psDevInfo->sDisplayInfo.ui32PhysicalHeightmm	= psPVRConnector->sConnector.display_info.height_mm;

	return PSB_OK;
}

#endif /* #if defined(SUPPORT_DRI_DRM) */

/******************************************************************************
 End of file (poulsbo_linux_crt.c)
******************************************************************************/
