/*************************************************************************/ /*!
@Title          Poulsbo linux-specific SDVO functions.
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

#if defined(SUPPORT_DRI_DRM)

static IMG_UINT32 SDVOGetDotClockMultiplier(IMG_UINT32 ui32DotClock)
{
	if (ui32DotClock >= 100000)
	{
		return 1;
	}
	else if (ui32DotClock >= 50000)
	{
		return 2;
	}
	else if (ui32DotClock >= 25000)
	{
		return 4;
	}
	else /* ui32DotClock >= 20000 */
	{
		return 5;
	}
}

static PSB_BOOL I2CReadByte(struct i2c_adapter *psAdapter, IMG_UINT8 ui8Op, IMG_UINT8 *pui8Byte)
{
	struct i2c_algo_bit_data *psAlgoData	= (struct i2c_algo_bit_data *)psAdapter->algo_data;
	PVRI2C_INFO *psI2CInfo			= (PVRI2C_INFO *)psAlgoData->data;
	struct i2c_msg sMessage[2];
	IMG_UINT8 ui8OutBuffer;
	IMG_UINT8 ui8InBuffer;

	ui8OutBuffer		= ui8Op;

	sMessage[0].addr	= psI2CInfo->ui32Addr;
	sMessage[0].flags	= 0;
	sMessage[0].len		= 1;
	sMessage[0].buf		= &ui8OutBuffer;

	sMessage[1].addr	= psI2CInfo->ui32Addr;
	sMessage[1].flags	= I2C_M_RD;
	sMessage[1].len		= 1;
	sMessage[1].buf		= &ui8InBuffer;

	if (i2c_transfer(psAdapter, &sMessage[0], 2) == 2)
	{
		*pui8Byte = ui8InBuffer;

		return PSB_TRUE;
	}
	else
	{
		return PSB_FALSE;
	}
}

static PSB_BOOL I2CWriteByte(struct i2c_adapter *psAdapter, IMG_UINT8 ui8Op, IMG_UINT8 ui8Byte)
{
	struct i2c_algo_bit_data *psAlgoData	= (struct i2c_algo_bit_data *)psAdapter->algo_data;
	PVRI2C_INFO *psI2CInfo			= (PVRI2C_INFO *)psAlgoData->data;
	struct i2c_msg sMessage;
	IMG_UINT8 aui8Buffer[2];

	aui8Buffer[0]	= ui8Op;
	aui8Buffer[1]	= ui8Byte;

	sMessage.addr	= psI2CInfo->ui32Addr;
	sMessage.flags	= 0;
	sMessage.len	= 2;
	sMessage.buf	= &aui8Buffer[0];

	return (i2c_transfer(psAdapter, &sMessage, 1) == 1) ? PSB_TRUE : PSB_FALSE;
}

static PSB_BOOL I2CCheckDeviceStatus(struct i2c_adapter *psAdapter)
{
	IMG_UINT8 ui8Status = 0;

	/* Read back the status */
	if (I2CReadByte(psAdapter, PVRPSB_I2C_STATUS, &ui8Status) == PSB_FALSE)
	{
		return PSB_FALSE;
	}

	return (ui8Status == 1) ? PSB_TRUE : PSB_FALSE;
}

static PSB_BOOL I2CCommandRead(struct i2c_adapter *psAdapter, IMG_UINT8 ui8Command, IMG_UINT8 *pui8Buffer, IMG_UINT32 ui32BufferLength)
{
	IMG_UINT8 *pui8TempBuffer;
	IMG_UINT32 ui32I;
	PSB_BOOL bReturn = PSB_FALSE;

	/* Allocate temp buffer to read data into */
	pui8TempBuffer = (IMG_UINT8 *)PVROSAllocKernelMem(ui32BufferLength);
	if (pui8TempBuffer == NULL)
	{
		return PSB_FALSE;
	}

	/* Tell the device what data we want */
	if (I2CWriteByte(psAdapter, PVRPSB_I2C_CMD, ui8Command) == PSB_FALSE)
	{
		goto ExitFreeTempBuffer;
	}

	/* Read the data back into a temporary buffer */
	for (ui32I = 0; ui32I < ui32BufferLength; ui32I++)
	{
		if (I2CReadByte(psAdapter, PVRPSB_I2C_RETURN0 + ui32I, &pui8TempBuffer[ui32I]) == PSB_FALSE)
		{
			goto ExitFreeTempBuffer;
		}
	}

	/* If all went well copy the data */
	if (I2CCheckDeviceStatus(psAdapter) == PSB_TRUE)
	{
		memcpy(pui8Buffer, pui8TempBuffer, ui32BufferLength);

		bReturn = PSB_TRUE;
	}

ExitFreeTempBuffer:
	PVROSFreeKernelMem(pui8TempBuffer);

	return bReturn;
}

static PSB_BOOL I2CCommandWrite(struct i2c_adapter *psAdapter, IMG_UINT8 ui8Command, IMG_UINT8 *pui8Buffer, IMG_UINT32 ui32BufferLength)
{
	IMG_UINT32 ui32I;

	/* Send the data to the device */
	for (ui32I = 0; ui32I < ui32BufferLength; ui32I++)
	{
		if (I2CWriteByte(psAdapter, PVRPSB_I2C_ARG0 - ui32I, pui8Buffer[ui32I]) == PSB_FALSE)
		{
			return PSB_FALSE;
		}
	}

	/* Tell the device what the data was that we just sent */
	if (I2CWriteByte(psAdapter, PVRPSB_I2C_CMD, ui8Command) == PSB_FALSE)
	{
		return PSB_FALSE;
	}

	return I2CCheckDeviceStatus(psAdapter);
}

static PSB_BOOL I2CGetActiveEncoders(struct i2c_adapter *psAdapter, IMG_UINT16 *pui16Outputs)
{
	IMG_UINT16 ui16ActiveEncoders;

	if (I2CCommandRead(psAdapter, PVRPSB_I2C_CMD_GETACTIVEENCODERS, (IMG_UINT8 *)&ui16ActiveEncoders, sizeof(ui16ActiveEncoders)) == PSB_TRUE)
	{
		*pui16Outputs = ui16ActiveEncoders;
		return PSB_TRUE;
	}

	return PSB_FALSE;
}

static PSB_BOOL I2CSetActiveEncoders(struct i2c_adapter *psAdapter, IMG_UINT16 ui16Outputs)
{
	return I2CCommandWrite(psAdapter, PVRPSB_I2C_CMD_SETACTIVEENCODERS, (IMG_UINT8 *)&ui16Outputs, sizeof(ui16Outputs));
}

static PSB_BOOL I2CSetInOutMap(struct i2c_adapter *psAdapter, IMG_UINT16 ui16Input0Output, IMG_UINT16 ui16Input1Output)
{
	IMG_UINT8 aui8InOutMap[4];

	aui8InOutMap[0] = ui16Input0Output & 0xFF;
	aui8InOutMap[1] = (ui16Input0Output & 0xFF00) >> 8;
	aui8InOutMap[2] = ui16Input1Output & 0xFF;
	aui8InOutMap[3] = (ui16Input1Output & 0xFF00) >> 8;

	return I2CCommandWrite(psAdapter, PVRPSB_I2C_CMD_SETINOUTMAP, &aui8InOutMap[0], sizeof(aui8InOutMap));
}

static PSB_BOOL I2CGetAttachedDisplays(struct i2c_adapter *psAdapter, IMG_UINT8 *pui8Output0, IMG_UINT8 *pui8Output1)
{
	IMG_UINT8 aui8Output[2];

	if (I2CCommandRead(psAdapter, PVRPSB_I2C_CMD_GETATTACHEDDISPLAYS, &aui8Output[0], sizeof(aui8Output)) == PSB_TRUE)
	{
		*pui8Output0 = aui8Output[0];
		*pui8Output1 = aui8Output[1];

		return PSB_TRUE;
	}

	return PSB_FALSE;
}

static PSB_BOOL I2CSetTargetInput(struct i2c_adapter *psAdapter, IMG_UINT8 ui8Input)
{
	PVRPSB_TARGET_INPUT sTarget = {0};

	sTarget.target = ui8Input;

	return I2CCommandWrite(psAdapter, PVRPSB_I2C_CMD_SETTARGETINPUT, (IMG_UINT8 *)&sTarget, sizeof(sTarget));
}

static PSB_BOOL I2CSetTargetOutput(struct i2c_adapter *psAdapter, IMG_UINT16 ui16Output)
{
	return I2CCommandWrite(psAdapter, PVRPSB_I2C_CMD_SETTARGETOUTPUT, (IMG_UINT8 *)&ui16Output, sizeof(ui16Output));
}

static PSB_BOOL I2CGetDeviceCapabilities(struct i2c_adapter *psAdapter, PVRPSB_DEVICE_CAPS *psDeviceCaps)
{
	return I2CCommandRead(psAdapter, PVRPSB_I2C_CMD_GETDEVICECAPS, (IMG_UINT8 *)psDeviceCaps, sizeof(PVRPSB_DEVICE_CAPS));
}

#if 0
/* Keep for future reference but silence compiler warnings */
static PSB_BOOL I2CGetInputTimings(struct i2c_adapter *psAdapter, PVRPSB_TIMINGS1 *psTimings1, PVRPSB_TIMINGS2 *psTimings2)
{
	if (I2CCommandRead(psAdapter, PVRPSB_I2C_CMD_GETINPUTTIMINGS1, (IMG_UINT8 *)psTimings1, sizeof(PVRPSB_TIMINGS1)) == PSB_TRUE)
	{
		return I2CCommandRead(psAdapter, PVRPSB_I2C_CMD_GETINPUTTIMINGS2, (IMG_UINT8 *)psTimings2, sizeof(PVRPSB_TIMINGS2));
	}
	else
	{
		return PSB_FALSE;
	}
}
#endif

static PSB_BOOL I2CSetInputTimings(struct i2c_adapter *psAdapter, PVRPSB_TIMINGS1 *psTimings1, PVRPSB_TIMINGS2 *psTimings2)
{
	if (I2CCommandWrite(psAdapter, PVRPSB_I2C_CMD_SETINPUTTIMINGS1, (IMG_UINT8 *)psTimings1, sizeof(PVRPSB_TIMINGS1)) == PSB_TRUE)
	{
		return I2CCommandWrite(psAdapter, PVRPSB_I2C_CMD_SETINPUTTIMINGS2, (IMG_UINT8 *)psTimings2, sizeof(PVRPSB_TIMINGS2));
	}
	else
	{
		return PSB_FALSE;
	}

	return PSB_TRUE;
}

#if 0
/* Keep for future reference but silence compiler warnings */
static PSB_BOOL I2CGetOutputTimings(struct i2c_adapter *psAdapter, PVRPSB_TIMINGS1 *psTimings1, PVRPSB_TIMINGS2 *psTimings2)
{
	if (I2CCommandRead(psAdapter, PVRPSB_I2C_CMD_GETOUTPUTTIMINGS1, (IMG_UINT8 *)psTimings1, sizeof(PVRPSB_TIMINGS1)) == PSB_TRUE)
	{
		return I2CCommandRead(psAdapter, PVRPSB_I2C_CMD_GETOUTPUTTIMINGS2, (IMG_UINT8 *)psTimings2, sizeof(PVRPSB_TIMINGS2));
	}
	else
	{
		return PSB_FALSE;
	}
}
#endif

static PSB_BOOL I2CSetOutputTimings(struct i2c_adapter *psAdapter, PVRPSB_TIMINGS1 *psTimings1, PVRPSB_TIMINGS2 *psTimings2)
{
	if (I2CCommandWrite(psAdapter, PVRPSB_I2C_CMD_SETOUTPUTTIMINGS1, (IMG_UINT8 *)psTimings1, sizeof(PVRPSB_TIMINGS1)) == PSB_TRUE)
	{
		return I2CCommandWrite(psAdapter, PVRPSB_I2C_CMD_SETOUTPUTTIMINGS2, (IMG_UINT8 *)psTimings2, sizeof(PVRPSB_TIMINGS2));
	}
	else
	{
		return PSB_FALSE;
	}
}

#if 0
/* Keep for future reference but silence compiler warnings */
static PSB_BOOL I2CGetClockMultiplier(struct i2c_adapter *psAdapter, IMG_UINT8 *pui8Multiplier)
{
	IMG_UINT8 ui8Multiplier;

	/* Tell the device we want the clock multiplier */
	if (I2CWriteByte(psAdapter, PVRPSB_I2C_CMD, PVRPSB_I2C_CMD_GETCLOCKMULTI) == PSB_FALSE)
	{
		return PSB_FALSE;
	}

	/* Read back the data */
	if (I2CReadByte(psAdapter, PVRPSB_I2C_RETURN0, &ui8Multiplier) == PSB_FALSE)
	{
		return PSB_FALSE;
	}

	/* If all went well copy the data */
	if (I2CCheckDeviceStatus(psAdapter) == PSB_TRUE)
	{
		*pui8Multiplier = ui8Multiplier;

		return PSB_TRUE;
	}
	else
	{
		return PSB_FALSE;
	}
}
#endif

static PSB_BOOL I2CSetClockMultiplier(struct i2c_adapter *psAdapter, IMG_UINT8 ui8Multiplier)
{
	/* Write the data first */
	if (I2CWriteByte(psAdapter, PVRPSB_I2C_ARG0, ui8Multiplier) == PSB_FALSE)
	{
		return PSB_FALSE;
	}

	/* Tell the device we want to set the clock multiplier */
	if (I2CWriteByte(psAdapter, PVRPSB_I2C_CMD, PVRPSB_I2C_CMD_SETCLOCKMULTI) == PSB_FALSE)
	{
		return PSB_FALSE;
	}

	return I2CCheckDeviceStatus(psAdapter);
}


/******************************************************************************
 * Connector functions
 ******************************************************************************/

static int SDVOConnectorHelperGetModes(struct drm_connector *psConnector)
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

static int SDVOConnectorHelperModeValid(struct drm_connector *psConnector, struct drm_display_mode *psMode)
{
	PVRPSB_DEVINFO *psDevInfo		= (PVRPSB_DEVINFO *)psConnector->dev->dev_private;
	const PVRPSB_PLL_RANGE *psPllRange	= NULL;
	int iModeStatus				= MODE_BAD;

	switch (psDevInfo->eDevice)
	{
		case PSB_CEDARVIEW:
			psPllRange = &sCdvNonLvds96PllRange;
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

static struct drm_encoder *SDVOConnectorHelperBestEncoder(struct drm_connector *psConnector)
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

static struct drm_connector_helper_funcs sSDVOConnectorHelperFuncs = 
{
	.get_modes	= SDVOConnectorHelperGetModes, 
	.mode_valid	= SDVOConnectorHelperModeValid, 
	.best_encoder	= SDVOConnectorHelperBestEncoder, 
};

static void SDVOConnectorSave(struct drm_connector *psConnector)
{
	/* Rely on doing a modeset as part of the CRTC restore. */
}

static void SDVOConnectorRestore(struct drm_connector *psConnector)
{
	/* Rely on doing a modeset as part of the CRTC restore. */
}

static enum drm_connector_status SDVOConnectorDetect(struct drm_connector *psConnector, bool force)
{
	PVRPSB_CONNECTOR *psPVRConnector = to_pvr_connector(psConnector);
	IMG_UINT8 aui8Outputs[2];

	PVR_UNREFERENCED_PARAMETER(force);

	if (I2CGetAttachedDisplays(psPVRConnector->psAdapter, &aui8Outputs[0], &aui8Outputs[1]) == PSB_FALSE)
	{
		printk(KERN_ERR DRVNAME " - %s: failed to get connector status.\n", __FUNCTION__);
		return connector_status_unknown;
	}

	if (aui8Outputs[0] != PVRPSB_ENCODER_NONE || aui8Outputs[1] != PVRPSB_ENCODER_NONE)
	{
		return connector_status_connected;
	}
	else
	{
		return connector_status_disconnected;
	}
}

static int SDVOConnectorSetProperty(struct drm_connector *psConnector, struct drm_property *psProperty, uint64_t ui64Val)
{
	return -ENOSYS;
}

static void SDVOConnectorDestroy(struct drm_connector *psConnector)
{
	PVRPSB_CONNECTOR *psPVRConnector = to_pvr_connector(psConnector);

	drm_mode_connector_update_edid_property(psConnector, NULL);
	drm_connector_cleanup(psConnector);

	PVRI2CAdapterDestroy(psPVRConnector->psAdapter);

	PVROSFreeKernelMem(psPVRConnector);
}

static void SDVOConnectorForce(struct drm_connector *psConnector)
{
	/* Not supported */
}

static const struct drm_connector_funcs sSDVOConnectorFuncs = 
{
	.dpms		= drm_helper_connector_dpms, 
	.save		= SDVOConnectorSave, 
	.restore	= SDVOConnectorRestore, 
	.detect		= SDVOConnectorDetect, 
	.fill_modes	= drm_helper_probe_single_connector_modes, 
	.set_property	= SDVOConnectorSetProperty, 
	.destroy	= SDVOConnectorDestroy, 
	.force		= SDVOConnectorForce, 
};

static PVRPSB_CONNECTOR *SDVOConnectorCreate(PVRPSB_DEVINFO *psDevInfo, struct i2c_adapter *psAdapter, PVRPSB_PORT ePort, int iType)
{
	PVRPSB_CONNECTOR *psPVRConnector;

	psPVRConnector = (PVRPSB_CONNECTOR *)kzalloc(sizeof(PVRPSB_CONNECTOR), GFP_KERNEL);
	if (psPVRConnector)
	{
		drm_connector_init(psDevInfo->psDrmDev, &psPVRConnector->sConnector, &sSDVOConnectorFuncs, iType);
		drm_connector_helper_add(&psPVRConnector->sConnector, &sSDVOConnectorHelperFuncs);

		psPVRConnector->psAdapter	= psAdapter;
		psPVRConnector->ePort		= ePort;

		I2CGetActiveEncoders(psPVRConnector->psAdapter, &psPVRConnector->ui16ActiveEncoders);
	}

	return psPVRConnector;
}

/******************************************************************************
 * Encoder functions
 ******************************************************************************/

static void SDVOEncoderHelperDpms(struct drm_encoder *psEncoder, int iMode)
{
	struct drm_connector *psConnector;
	PVRPSB_CONNECTOR *psPVRConnector;
	IMG_UINT16 ui16ActiveEncoders = PVRPSB_ENCODER_NONE;

	psConnector = PVRGetConnectorForEncoder(psEncoder);
	if (psConnector == NULL)
	{
		printk(KERN_ERR DRVNAME " - %s: The given encoder isn't attached to a connector.\n", __FUNCTION__);
		return;
	}
	psPVRConnector = to_pvr_connector(psConnector);

	if (I2CGetActiveEncoders(psPVRConnector->psAdapter, &ui16ActiveEncoders) == PSB_FALSE)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to get active outputs.\n", __FUNCTION__);
		return;
	}

	switch (iMode)
	{
		case DRM_MODE_DPMS_ON:
		{
			if (psPVRConnector->ePort == PSB_PORT_SDVO_B)
			{
				ui16ActiveEncoders = (ui16ActiveEncoders & 0xF0) | PVRPSB_ENCODER_TMDS0;
			}
			else
			{
				ui16ActiveEncoders = (ui16ActiveEncoders & 0x0F) | PVRPSB_ENCODER_TMDS1;
			}
			break;
		}
		case DRM_MODE_DPMS_STANDBY:
		case DRM_MODE_DPMS_SUSPEND:
		case DRM_MODE_DPMS_OFF:
		{
			if (psPVRConnector->ePort == PSB_PORT_SDVO_B)
			{
				ui16ActiveEncoders = (ui16ActiveEncoders & 0xF0) | PVRPSB_ENCODER_NONE;
			}
			else
			{
				ui16ActiveEncoders = (ui16ActiveEncoders & 0x0F) | PVRPSB_ENCODER_NONE;
			}
			break;
		}
		default:
		{
			return;
		}
	}

	if (I2CSetActiveEncoders(psPVRConnector->psAdapter, ui16ActiveEncoders) == PSB_FALSE)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to set DPMS mode\n", __FUNCTION__);
	}
	else
	{
		psPVRConnector->ui16ActiveEncoders = ui16ActiveEncoders;
	}
}

static bool SDVOEncoderHelperModeFixup(struct drm_encoder *psEncoder, const struct drm_display_mode *psMode, struct drm_display_mode *psAdjustedMode)
{
	/* Adjust the clock so it's within range */
	psAdjustedMode->clock *= SDVOGetDotClockMultiplier(psMode->clock);

	return true;
}

static void SDVOEncoderHelperPrepare(struct drm_encoder *psEncoder)
{
	PVRPSB_DEVINFO *psDevInfo = (PVRPSB_DEVINFO *)psEncoder->dev->dev_private;
	PVRPSB_CONNECTOR *psPVRConnector;
	struct drm_connector *psConnector;
	IMG_UINT16 ui16ActiveEncoders;
	IMG_UINT32 ui32RegVal;

	/* Disable port */
	ui32RegVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_SDVOB_CTL);
	ui32RegVal = PVRPSB_SDVO_CTL_ENABLE_SET(ui32RegVal, 0);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_SDVOB_CTL, ui32RegVal);

	/* Disable the encoder */
	psConnector = PVRGetConnectorForEncoder(psEncoder);
	if (psConnector)
	{
		psPVRConnector = to_pvr_connector(psConnector);

		if (psPVRConnector->ePort == PSB_PORT_SDVO_B)
		{
			ui16ActiveEncoders = (psPVRConnector->ui16ActiveEncoders & 0xF0) | PVRPSB_ENCODER_NONE;
		}
		else
		{
			ui16ActiveEncoders = (psPVRConnector->ui16ActiveEncoders & 0x0F) | PVRPSB_ENCODER_NONE;
		}

		I2CSetActiveEncoders(psPVRConnector->psAdapter, ui16ActiveEncoders);
	}
	else
	{
		printk(KERN_WARNING DRVNAME " - %s: The given encoder isn't attached to a connector.\n", __FUNCTION__);
	}
}

static void SDVOEncoderHelperCommit(struct drm_encoder *psEncoder)
{
	PVRPSB_DEVINFO *psDevInfo = (PVRPSB_DEVINFO *)psEncoder->dev->dev_private;
	PVRPSB_CONNECTOR *psPVRConnector;
	struct drm_connector *psConnector;
	IMG_UINT32 ui32RegVal;

	/* Enable the encoder */
	psConnector = PVRGetConnectorForEncoder(psEncoder);
	if (psConnector)
	{
		psPVRConnector = to_pvr_connector(psConnector);

		if (I2CSetActiveEncoders(psPVRConnector->psAdapter, psPVRConnector->ui16ActiveEncoders) == PSB_FALSE)
		{
			printk(KERN_ERR DRVNAME " - %s: Failed to enable the encoder.\n", __FUNCTION__);
		}
	}
	else
	{
		printk(KERN_WARNING DRVNAME " - %s: The given encoder isn't attached to a connector.\n", __FUNCTION__);
	}

	/* Enable port */
	ui32RegVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_SDVOB_CTL);
	ui32RegVal = PVRPSB_SDVO_CTL_ENABLE_SET(ui32RegVal, 1);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_SDVOB_CTL, ui32RegVal);
}

static void SDVOEncoderHelperModeSet(struct drm_encoder *psEncoder, struct drm_display_mode *psMode, struct drm_display_mode *psAdjustedMode)
{
	PVRPSB_DEVINFO *psDevInfo		= (PVRPSB_DEVINFO *)psEncoder->dev->dev_private;
	PVRPSB_ENCODER *psPVREncoder		= to_pvr_encoder(psEncoder);
	PVRPSB_CRTC *psPVRCrtc			= to_pvr_crtc(psEncoder->crtc);
	PVRPSB_CONNECTOR *psPVRConnector;
	struct drm_connector *psConnector;
	const IMG_UINT32 ui32DpllCtlReg		= (psPVRCrtc->ePipe == PSB_PIPE_A) ? PVRPSB_DPLLA_CTL : PVRPSB_DPLLB_CTL;
	const IMG_UINT32 ui32Fp0Reg		= (psPVRCrtc->ePipe == PSB_PIPE_A) ? PVRPSB_FPA0 : PVRPSB_FPB0;
	const PVRPSB_PLL_RANGE *psPllRange	= NULL;
	PLL_FREQ sPllFreqInfo;
	IMG_UINT32 ui32ClockMultiplier;
	IMG_UINT32 ui32PllCtl;
	IMG_UINT32 ui32Fp0;
	PVRPSB_TIMINGS1 sTimings1;
	PVRPSB_TIMINGS2 sTimings2;
	IMG_UINT16 ui16HBlankPeriod;
	IMG_UINT16 ui16VBlankPeriod;
	IMG_UINT16 ui16HFrontPorchPeriod;
	IMG_UINT16 ui16VFrontPorchPeriod;
	IMG_UINT16 ui16HSyncPeriod;
	IMG_UINT16 ui16VSyncPeriod;
	IMG_UINT32 ui32Width;
	IMG_UINT16 ui32Height;

	/* Before we attempt to do anything get the connector to which the encoder is attached */
	psConnector = PVRGetConnectorForEncoder(psEncoder);
	if (psConnector == NULL)
	{
		printk(KERN_ERR DRVNAME " - %s: The given encoder isn't attached to a connector.\n", __FUNCTION__);
		return;
	}
	psPVRConnector = to_pvr_connector(psConnector);

	switch (psDevInfo->eDevice)
	{
		case PSB_CEDARVIEW:
			psPllRange = &sCdvNonLvds96PllRange;
			break;
		case PSB_POULSBO:
			psPllRange = &sPsbNonLvdsPllRange;
			break;
		case PSB_UNKNOWN:
			printk(KERN_ERR DRVNAME " - %s: Unknown device\n", __FUNCTION__);
			return;
	}

	/* Determine PLL data for the given clock */
	if (PVRPSBSelectPLLFreq(psAdjustedMode->clock, psPllRange, &sPllFreqInfo) == PSB_FALSE)
	{
		printk(KERN_ERR DRVNAME " - %s: PVRPSBSelectPLLFreq failed\n", __FUNCTION__);
		return;
	}

	/* Program PLL related registers */
	ui32ClockMultiplier = psAdjustedMode->clock / psMode->clock;

	ui32Fp0 = 0;
	ui32Fp0 = PVRPSB_FP_NDIVISOR_SET(ui32Fp0, sPllFreqInfo.ui32N);
	ui32Fp0 = PVRPSB_FP_M1DIVISOR_SET(ui32Fp0, sPllFreqInfo.ui32M1);
	ui32Fp0 = PVRPSB_FP_M2DIVISOR_SET(ui32Fp0, sPllFreqInfo.ui32M2);

	ui32PllCtl = 0;
	ui32PllCtl = PVRPSB_DPLL_ENABLE_SET(ui32PllCtl, 0);
	ui32PllCtl = PVRPSB_DPLL_HIGHSPEED_ENABLE_SET(ui32PllCtl, 1);
	ui32PllCtl = PVRPSB_DPLL_VGA_DISABLE_SET(ui32PllCtl, 1);
	ui32PllCtl = PVRPSB_DPLL_MODE_SET(ui32PllCtl, PVRPSB_DPLL_MODE_SDVO);
	ui32PllCtl = PVRPSB_DPLL_P1_POSTDIVIDE_SET(ui32PllCtl, (1 << (sPllFreqInfo.ui32P1 - 1)));
	ui32PllCtl = PVRPSB_DPLL_REF_CLOCK_SET(ui32PllCtl, PVRPSB_DPLL_REF_CLOCK_DEFAULT);
	ui32PllCtl = PVRPSB_DPLL_CLOCK_MULTI_SET(ui32PllCtl, ui32ClockMultiplier);
	ui32PllCtl = PVRPSB_DPLL_P2_SET(ui32PllCtl, ((10 - sPllFreqInfo.ui32P2) / 5));

	if (sPllFreqInfo.ui32P2 == sCdvNonLvds27PllRange.ui32P2Hi)
	{
		ui32PllCtl = PVRPSB_DPLL_P2_DIVIDE_SET(ui32PllCtl, PVRPSB_DPLL_P2_DIVIDE_SDVO_10);
	}
	else
	{
		ui32PllCtl = PVRPSB_DPLL_P2_DIVIDE_SET(ui32PllCtl, PVRPSB_DPLL_P2_DIVIDE_SDVO_5);
	}

	/* We have to do this odd sequence to get the values to stick... */
	PVROSWriteMMIOReg(psDevInfo, ui32Fp0Reg, ui32Fp0);
	PVROSWriteMMIOReg(psDevInfo, ui32DpllCtlReg, ui32PllCtl);

	PVROSDelayus(150);

	PVROSWriteMMIOReg(psDevInfo, ui32Fp0Reg, ui32Fp0);

	ui32PllCtl = PVRPSB_DPLL_ENABLE_SET(ui32PllCtl, 1);
	PVROSWriteMMIOReg(psDevInfo, ui32DpllCtlReg, ui32PllCtl);
	ui32PllCtl = PVROSReadMMIOReg(psDevInfo, ui32DpllCtlReg);
	
	PVROSDelayus(150);

	PVROSWriteMMIOReg(psDevInfo, ui32DpllCtlReg, ui32PllCtl);
	PVROSReadMMIOReg(psDevInfo, ui32DpllCtlReg);
	PVROSDelayus(150);


	/* Configure the SDVO chip with the new display mode parameters */
	ui32Width		= drm_mode_width(psAdjustedMode);
	ui32Height		= drm_mode_height(psAdjustedMode);

	ui16HBlankPeriod	= psAdjustedMode->htotal - ui32Width;
	ui16VBlankPeriod	= psAdjustedMode->htotal - ui32Height;

	ui16HFrontPorchPeriod	= psAdjustedMode->hsync_start - ui32Width;
	ui16VFrontPorchPeriod	= psAdjustedMode->vsync_start - ui32Height;

	ui16HSyncPeriod		= psAdjustedMode->hsync_end - psAdjustedMode->hsync_start;
	ui16VSyncPeriod		= psAdjustedMode->vsync_end - psAdjustedMode->vsync_start;

	/* Setup encoder mode data 1 */
	sTimings1.ui16Clock		= (IMG_UINT16)(psMode->clock / 10);

	sTimings1.ui8WidthLower		= ui32Width & 0xFF;
	sTimings1.ui8HBlankLower	= ui16HBlankPeriod & 0xFF;
	sTimings1.ui8WidthHBlankHigh	= ((ui32Width & 0xF00) >> 4) | ((ui16HBlankPeriod & 0xF00) >> 8);

	sTimings1.ui8HeightLower	= ui32Height & 0xFF;
	sTimings1.ui8VBlankLower	= ui16VBlankPeriod & 0xFF;
	sTimings1.ui8HeightVBlankHigh	= ((ui32Height & 0xF00) >> 4) | ((ui16VBlankPeriod & 0xF00) >> 8);


	/* Setup encoder mode data 2 */
	sTimings2.ui8HFrontPorchLower	= ui16HFrontPorchPeriod & 0xFF;
	sTimings2.ui8HSyncLower		= ui16HSyncPeriod & 0xFF;
	sTimings2.ui8VFrontPorchSync	= ((ui16VFrontPorchPeriod & 0xF) << 4) | (ui16VSyncPeriod & 0xF);
	sTimings2.ui8FrontPorchSyncHigh	= ((ui16HFrontPorchPeriod & 0x300) >> 2) | ((ui16HSyncPeriod & 0x300) >> 4) | 
		((ui16VFrontPorchPeriod & 0x30) >> 2) | ((ui16VSyncPeriod & 0x30) >> 4);
	sTimings2.ui8VFrontPorchHigh	= ui16VFrontPorchPeriod & 0xC0;

	sTimings2.ui8DTDFlags		= 0x18; /* Magic */

	if ((psAdjustedMode->flags & DRM_MODE_FLAG_PHSYNC) != 0)
	{
		sTimings2.ui8DTDFlags	|= 0x2;
	}

	if ((psAdjustedMode->flags & DRM_MODE_FLAG_PVSYNC) != 0)
	{
		sTimings2.ui8DTDFlags	|= 0x4;
	}

	sTimings2.ui8SDVOFlags		= 0;
	sTimings2.ui8Padding		= 0;

	I2CSetInOutMap(psPVRConnector->psAdapter, psPVRConnector->ui16ActiveEncoders, PVRPSB_ENCODER_NONE);

	/* Set the input and output timings */
	if (psPVRConnector->ePort == PSB_PORT_SDVO_B)
	{
		I2CSetTargetInput(psPVRConnector->psAdapter, 0);

		if (I2CSetInputTimings(psPVRConnector->psAdapter, &sTimings1, &sTimings2) == PSB_FALSE)
		{
			printk(KERN_ERR DRVNAME " - %s: Failed to set encoder input timings.\n", __FUNCTION__);
		}
	}
	else
	{
		printk(KERN_ERR DRVNAME " - %s: Couldn't set encoder input timings for unsupported port.\n", __FUNCTION__);
	}

	I2CSetTargetOutput(psPVRConnector->psAdapter, psPVREncoder->ui16OutputType);
	if (I2CSetOutputTimings(psPVRConnector->psAdapter, &sTimings1, &sTimings2) == PSB_FALSE)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to set encoder output timings.\n", __FUNCTION__);
	}

	/* Set the clock rate multiplier */
	if (I2CSetClockMultiplier(psPVRConnector->psAdapter, 0x1 << (ui32ClockMultiplier - 1)) == PSB_FALSE)
	{
		printk(KERN_ERR DRVNAME " - %s: Failed to set encoder output timings.\n", __FUNCTION__);
	}
}

static const struct drm_encoder_helper_funcs sSDVOEncoderHelperFuncs = 
{
	.dpms		= SDVOEncoderHelperDpms, 
	.save		= NULL, 
	.restore	= NULL, 
	.mode_fixup	= SDVOEncoderHelperModeFixup, 
	.prepare	= SDVOEncoderHelperPrepare, 
	.commit		= SDVOEncoderHelperCommit, 
	.mode_set	= SDVOEncoderHelperModeSet, 
	.get_crtc	= NULL, 
	.detect		= NULL, 
	.disable	= NULL, 
};

static void SDVOEncoderDestroy(struct drm_encoder *psEncoder)
{
	PVRPSB_ENCODER *psPVREncoder = to_pvr_encoder(psEncoder);

	drm_encoder_cleanup(psEncoder);
	PVROSFreeKernelMem(psPVREncoder);
}

static const struct drm_encoder_funcs sSDVOEncoderFuncs = 
{
	.destroy = SDVOEncoderDestroy, 
};

static PVRPSB_ENCODER *SDVOEncoderCreate(PVRPSB_DEVINFO *psDevInfo, IMG_UINT16 ui16OutputType)
{
	PVRPSB_ENCODER *psPVREncoder;

	psPVREncoder = (PVRPSB_ENCODER *)kzalloc(sizeof(PVRPSB_ENCODER), GFP_KERNEL);
	if (psPVREncoder)
	{
		int iEncoderType;

		switch (ui16OutputType)
		{
			case PVRPSB_ENCODER_TMDS0:
			{
				iEncoderType = DRM_MODE_ENCODER_TMDS;
				break;
			}
			default:
			{
				iEncoderType = DRM_MODE_ENCODER_NONE;
				break;
			}
		}

		drm_encoder_init(psDevInfo->psDrmDev, &psPVREncoder->sEncoder, &sSDVOEncoderFuncs, iEncoderType);
		drm_encoder_helper_add(&psPVREncoder->sEncoder, &sSDVOEncoderHelperFuncs);

		/* This is a bit field that's used to determine by which CRTCs the encoder can be driven. 
		   We have only one CRTC so always set to 0x1 */
		psPVREncoder->sEncoder.possible_crtcs = 0x1;

		psPVREncoder->ui16OutputType = ui16OutputType;
	}

	return psPVREncoder;
}

/******************************************************************************
 * Non-static functions
 ******************************************************************************/
PSB_ERROR SDVOSetup(PVRPSB_DEVINFO *psDevInfo)
{
	if (PVRPSB_SDVO_CTL_ENABLE_GET(PVROSReadMMIOReg(psDevInfo, PVRPSB_SDVOB_CTL)) != 0)
	{
		struct i2c_adapter *psAdapter;

		psAdapter = PVRI2CAdapterCreate(psDevInfo, "PSB I2C Adapter (SDVO)", PVRPSB_GPIO_E, PVRPSB_I2C_ADDR_SDVO_B);
		if (psAdapter == NULL)
		{
			printk(KERN_ERR DRVNAME " - %s: failed to create an I2C adapter.\n", __FUNCTION__);

			return PSB_ERROR_OUT_OF_MEMORY;
		}

		if (I2CGetDeviceCapabilities(psAdapter, &psDevInfo->sSdvoCapabilities) == PSB_FALSE)
		{
			printk(KERN_ERR DRVNAME " - %s: failed to get device capabilities.\n", __FUNCTION__);
			
			PVRI2CAdapterDestroy(psAdapter);
			return PSB_ERROR_GENERIC;
		}

		if (psDevInfo->sSdvoCapabilities.ui16OutputFlags & PVRPSB_ENCODER_TMDS0)
		{
			PVRPSB_CONNECTOR *psPVRConnector;
			PVRPSB_ENCODER *psPVREncoder;

			psPVRConnector = SDVOConnectorCreate(psDevInfo, psAdapter, PSB_PORT_SDVO_B, DRM_MODE_CONNECTOR_DVID);
			if (psPVRConnector == NULL)
			{
				printk(KERN_ERR DRVNAME " - %s: failed to create a DVID connector.\n", __FUNCTION__);
				
				PVRI2CAdapterDestroy(psAdapter);
				return PSB_ERROR_OUT_OF_MEMORY;
			}

			psPVREncoder = SDVOEncoderCreate(psDevInfo, PVRPSB_ENCODER_TMDS0);
			if (psPVREncoder == NULL)
			{
				printk(KERN_ERR DRVNAME " - %s: failed to create a TMDS0 encoder.\n", __FUNCTION__);
				
				return PSB_ERROR_OUT_OF_MEMORY;
			}
			drm_mode_connector_attach_encoder(&psPVRConnector->sConnector, &psPVREncoder->sEncoder);

			/* Encoders that aren't attached to a connector are turned off during initialisation 
			   but the default connector DPMS mode is set to 'on'. Unfortunately, the DRM code doesn't 
			   turn an encoder on when attaching it to a connector and it doesn't set the connector 
			   DPMS status to 'off'. This creates a problem if the DPMS property is set to 'on' from 
			   userspace since the requested DPMS status is checked against the current connector DPMS 
			   status, which will of course also be 'on', and so it returns without doing anything. 
			   To fix this we have to set the connector status here to match the encoder. */
			psPVRConnector->sConnector.dpms = DRM_MODE_DPMS_OFF;
			
			psDevInfo->sDisplayInfo.ui32PhysicalWidthmm	= psPVRConnector->sConnector.display_info.width_mm;
			psDevInfo->sDisplayInfo.ui32PhysicalHeightmm	= psPVRConnector->sConnector.display_info.height_mm;
		}
		else
		{
			printk(KERN_ERR DRVNAME " - %s: no supported outputs found.\n", __FUNCTION__);
			
			return PSB_ERROR_INIT_FAILURE;
		}
	}
	else
	{
		printk(KERN_ERR DRVNAME " - %s: no supported output ports found.\n", __FUNCTION__);
		
		return PSB_ERROR_INIT_FAILURE;
	}

	return PSB_OK;
}

#endif /* #if defined(SUPPORT_DRI_DRM) */

/******************************************************************************
 End of file (poulsbo_linux_sdvo.c)
******************************************************************************/
