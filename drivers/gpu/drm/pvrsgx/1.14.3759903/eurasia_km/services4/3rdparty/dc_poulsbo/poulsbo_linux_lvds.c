/*************************************************************************/ /*!
@Title          Poulsbo linux-specific LVDS functions.
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

/******************************************************************************
 * Connector functions
 ******************************************************************************/

static int LVDSConnectorHelperGetModes(struct drm_connector *psConnector)
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

static int LVDSConnectorHelperModeValid(struct drm_connector *psConnector, struct drm_display_mode *psMode)
{
	PVRPSB_DEVINFO *psDevInfo		= (PVRPSB_DEVINFO *)psConnector->dev->dev_private;
	const PVRPSB_PLL_RANGE *psPllRange	= NULL;
	int iModeStatus				= MODE_BAD;

	switch (psDevInfo->eDevice)
	{
		case PSB_CEDARVIEW:
			psPllRange = &sCdvSingleLvdsPllRange;
			break;
		case PSB_POULSBO:
			psPllRange = &sPsbSingleLvdsPllRange;
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

static struct drm_encoder *LVDSConnectorHelperBestEncoder(struct drm_connector *psConnector)
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

static struct drm_connector_helper_funcs sLVDSConnectorHelperFuncs = 
{
	.get_modes	= LVDSConnectorHelperGetModes, 
	.mode_valid	= LVDSConnectorHelperModeValid, 
	.best_encoder	= LVDSConnectorHelperBestEncoder, 
};

static void LVDSConnectorSave(struct drm_connector *psConnector)
{
}

static void LVDSConnectorRestore(struct drm_connector *psConnector)
{
}

static enum drm_connector_status LVDSConnectorDetect(struct drm_connector *psConnector, bool force)
{
	return connector_status_connected;
}

static int LVDSConnectorSetProperty(struct drm_connector *psConnector, struct drm_property *psProperty, uint64_t ui64Val)
{
	return -ENOSYS;
}

static void LVDSConnectorDestroy(struct drm_connector *psConnector)
{
	PVRPSB_CONNECTOR *psPVRConnector = to_pvr_connector(psConnector);

	drm_mode_connector_update_edid_property(psConnector, NULL);
	drm_connector_cleanup(psConnector);

	PVRI2CAdapterDestroy(psPVRConnector->psAdapter);

	PVROSFreeKernelMem(psPVRConnector);
}

static void LVDSConnectorForce(struct drm_connector *psConnector)
{
	/* Not supported */
}

static const struct drm_connector_funcs sLVDSConnectorFuncs = 
{
	.dpms		= drm_helper_connector_dpms, 
	.save		= LVDSConnectorSave, 
	.restore	= LVDSConnectorRestore, 
	.detect		= LVDSConnectorDetect, 
	.fill_modes	= drm_helper_probe_single_connector_modes, 
	.set_property	= LVDSConnectorSetProperty, 
	.destroy	= LVDSConnectorDestroy, 
	.force		= LVDSConnectorForce, 
};

static PVRPSB_CONNECTOR *LVDSConnectorCreate(PVRPSB_DEVINFO *psDevInfo, struct i2c_adapter *psAdapter)
{
	PVRPSB_CONNECTOR *psPVRConnector;

	psPVRConnector = (PVRPSB_CONNECTOR *)kzalloc(sizeof(PVRPSB_CONNECTOR), GFP_KERNEL);
	if (psPVRConnector)
	{
		drm_connector_init(psDevInfo->psDrmDev, &psPVRConnector->sConnector, &sLVDSConnectorFuncs, DRM_MODE_CONNECTOR_LVDS);
		drm_connector_helper_add(&psPVRConnector->sConnector, &sLVDSConnectorHelperFuncs);

		psPVRConnector->psAdapter	= psAdapter;
		psPVRConnector->ePort		= PSB_PORT_LVDS;
	}

	return psPVRConnector;
}

/******************************************************************************
 * Encoder functions
 ******************************************************************************/

static void LVDSEncoderHelperDpms(struct drm_encoder *psEncoder, int iMode)
{
	/* Nothing to do */

}

static bool LVDSEncoderHelperModeFixup(struct drm_encoder *psEncoder, const struct drm_display_mode *psMode, struct drm_display_mode *psAdjustedMode)
{
	/* Nothing to do */
	return true;
}

static void LVDSEncoderHelperPrepare(struct drm_encoder *psEncoder)
{
	PVRPSB_DEVINFO *psDevInfo = (PVRPSB_DEVINFO *)psEncoder->dev->dev_private;
	IMG_UINT32 ui32RegVal;

	/* Disable port */
	ui32RegVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_LVDS_CTL);
	ui32RegVal = PVRPSB_LVDS_CTL_ENABLE_SET(ui32RegVal, 0);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_LVDS_CTL, ui32RegVal);
}

static void LVDSEncoderHelperCommit(struct drm_encoder *psEncoder)
{
	PVRPSB_DEVINFO *psDevInfo = (PVRPSB_DEVINFO *)psEncoder->dev->dev_private;
	IMG_UINT32 ui32RegVal;

	/* Enable port */
	ui32RegVal = PVROSReadMMIOReg(psDevInfo, PVRPSB_LVDS_CTL);
	ui32RegVal = PVRPSB_LVDS_CTL_ENABLE_SET(ui32RegVal, 1);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_LVDS_CTL, ui32RegVal);
}

static void LVDSEncoderHelperModeSet(struct drm_encoder *psEncoder, struct drm_display_mode *psMode, struct drm_display_mode *psAdjustedMode)
{
	/* Nothing to do */
}

static const struct drm_encoder_helper_funcs sLVDSEncoderHelperFuncs = 
{
	.dpms		= LVDSEncoderHelperDpms, 
	.save		= NULL, 
	.restore	= NULL, 
	.mode_fixup	= LVDSEncoderHelperModeFixup, 
	.prepare	= LVDSEncoderHelperPrepare, 
	.commit		= LVDSEncoderHelperCommit, 
	.mode_set	= LVDSEncoderHelperModeSet, 
	.get_crtc	= NULL, 
	.detect		= NULL, 
	.disable	= NULL, 
};

static void LVDSEncoderDestroy(struct drm_encoder *psEncoder)
{
	PVRPSB_ENCODER *psPVREncoder = to_pvr_encoder(psEncoder);

	drm_encoder_cleanup(psEncoder);
	PVROSFreeKernelMem(psPVREncoder);
}

static const struct drm_encoder_funcs sLVDSEncoderFuncs = 
{
	.destroy = LVDSEncoderDestroy, 
};

static PVRPSB_ENCODER *LVDSEncoderCreate(PVRPSB_DEVINFO *psDevInfo)
{
	PVRPSB_ENCODER *psPVREncoder;

	psPVREncoder = (PVRPSB_ENCODER *)kzalloc(sizeof(PVRPSB_ENCODER), GFP_KERNEL);
	if (psPVREncoder)
	{
		drm_encoder_init(psDevInfo->psDrmDev, &psPVREncoder->sEncoder, &sLVDSEncoderFuncs, DRM_MODE_ENCODER_LVDS);
		drm_encoder_helper_add(&psPVREncoder->sEncoder, &sLVDSEncoderHelperFuncs);

		/* This is a bit field that's used to determine by which CRTCs the encoder can be driven. 
		   We have only one CRTC so always set to 0x1 */
		psPVREncoder->sEncoder.possible_crtcs = 0x1;
	}

	return psPVREncoder;
}

/******************************************************************************
 * Non-static functions
 ******************************************************************************/
PSB_ERROR LVDSSetup(PVRPSB_DEVINFO *psDevInfo)
{
	if (PVRPSB_LVDS_CTL_ENABLE_GET(PVROSReadMMIOReg(psDevInfo, PVRPSB_LVDS_CTL)) != 0)
	{
		struct i2c_adapter *psAdapter;
		PVRPSB_CONNECTOR *psPVRConnector;
		PVRPSB_ENCODER *psPVREncoder;

		psAdapter = PVRI2CAdapterCreate(psDevInfo, "PSB I2C Adapter (LVDS)", PVRPSB_GPIO_C, 0);
		if (psAdapter == NULL)
		{
			printk(KERN_ERR DRVNAME " - %s: failed to create an I2C adapter.\n", __FUNCTION__);

			return PSB_ERROR_OUT_OF_MEMORY;
		}

		psPVRConnector = LVDSConnectorCreate(psDevInfo, psAdapter);
		if (psPVRConnector == NULL)
		{
			printk(KERN_ERR DRVNAME " - %s: failed to create an LVDS connector.\n", __FUNCTION__);
				
			PVRI2CAdapterDestroy(psAdapter);
			return PSB_ERROR_OUT_OF_MEMORY;
		}

		psPVREncoder = LVDSEncoderCreate(psDevInfo);
		if (psPVREncoder == NULL)
		{
			printk(KERN_ERR DRVNAME " - %s: failed to create a LVDS encoder.\n", __FUNCTION__);

			return PSB_ERROR_OUT_OF_MEMORY;
		}
		drm_mode_connector_attach_encoder(&psPVRConnector->sConnector, &psPVREncoder->sEncoder);

		psDevInfo->sDisplayInfo.ui32PhysicalWidthmm	= psPVRConnector->sConnector.display_info.width_mm;
		psDevInfo->sDisplayInfo.ui32PhysicalHeightmm	= psPVRConnector->sConnector.display_info.height_mm;
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
 End of file (poulsbo_linux_lvds.c)
******************************************************************************/
