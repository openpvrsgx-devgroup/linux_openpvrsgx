/*
 * Copyright © 2011 Intel Corporation
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
 * Authors:
 *	jim liu <jim.liu@intel.com>
 */

#include <drm/drmP.h>
#include <drm/drm.h>
#include <drm/drm_crtc.h>
#include <drm/drm_edid.h>
#include "psb_intel_drv.h"
#include "psb_drv.h"
#include "psb_intel_reg.h"
#include "psb_intel_hdmi_reg.h"
#include "psb_intel_hdmi_edid.h"
#include "psb_intel_hdmi.h"
#include <linux/pm_runtime.h>

static void mdfld_hdmi_mode_set(struct drm_encoder *encoder,
				struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
	struct drm_device *dev = encoder->dev;
	struct psb_intel_output *output = enc_to_psb_intel_output(encoder);
	struct mid_intel_hdmi_priv *hdmi_priv = output->dev_priv;
	u32 hdmib;
	struct drm_crtc *crtc = encoder->crtc;
	struct psb_intel_crtc *intel_crtc = to_psb_intel_crtc(crtc);
	PSB_DEBUG_ENTRY("\n");

	hdmib = (2 << 10);

	if (adjusted_mode->flags & DRM_MODE_FLAG_PVSYNC)
		hdmib |= SDVO_VSYNC_ACTIVE_HIGH;
	if (adjusted_mode->flags & DRM_MODE_FLAG_PHSYNC)
		hdmib |= SDVO_HSYNC_ACTIVE_HIGH;

	if (intel_crtc->pipe == 1)
		hdmib |= HDMIB_PIPE_B_SELECT;

	if (hdmi_priv->has_hdmi_audio) {
		hdmib |= HDMI_AUDIO_ENABLE;
		hdmib |= HDMI_NULL_PACKETS_DURING_VSYNC;
	}
		
	REG_WRITE(hdmi_priv->hdmi_reg, hdmib);
	REG_READ(hdmi_priv->hdmi_reg);
}

static bool mdfld_hdmi_mode_fixup(struct drm_encoder *encoder,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{

       return true;
}


static void mdfld_hdmi_dpms(struct drm_encoder *encoder, int mode)
{
	struct drm_device *dev = encoder->dev;
	struct psb_intel_output *output = enc_to_psb_intel_output(encoder);
	struct mid_intel_hdmi_priv *hdmi_priv = output->dev_priv;
	u32 hdmib;

	PSB_DEBUG_ENTRY("%s \n", mode == DRM_MODE_DPMS_ON ? "on" : "off");

	hdmib = REG_READ(hdmi_priv->hdmi_reg);

	if (mode != DRM_MODE_DPMS_ON) {
		REG_WRITE(hdmi_priv->hdmi_reg, hdmib & ~HDMIB_PORT_EN);
	} else {
		REG_WRITE(hdmi_priv->hdmi_reg, hdmib | HDMIB_PORT_EN);
	}
	REG_READ(hdmi_priv->hdmi_reg);
}

static void mdfld_hdmi_save(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct psb_intel_output *output = to_psb_intel_output(connector);
	struct mid_intel_hdmi_priv *hdmi_priv = output->dev_priv;

	PSB_DEBUG_ENTRY("\n");

	hdmi_priv->save_HDMIB = REG_READ(hdmi_priv->hdmi_reg);
}

static void mdfld_hdmi_restore(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct psb_intel_output *output = to_psb_intel_output(connector);
	struct mid_intel_hdmi_priv *hdmi_priv = output->dev_priv;

	PSB_DEBUG_ENTRY("\n");

	REG_WRITE(hdmi_priv->hdmi_reg, hdmi_priv->save_HDMIB);
	REG_READ(hdmi_priv->hdmi_reg);
}

#if 0

/* HDMI DIP related stuff */
static int mdfld_hdmi_get_cached_edid_block(struct drm_connector *connector, uint32_t num_block, uint8_t *edid_block, uint32_t size)
{
    struct drm_display_info *displayinfo = &(connector->display_info);
    if (num_block >= MAX_EDID_BLOCKS)
    {
	DRM_ERROR("mdfld_hdmi_get_cached_edid_block() - Invalid EDID block\n");
        return 0; 
    }
    edid_block = &displayinfo->raw_edid[EDID_BLOCK_SIZE*num_block];
    return 1;
}

/////////////////////////////////////////////////////////////////////////
//    INTHDMIENCODER_CreateEELDPacket():
//    This function parses v1.3 base EDID and CEA-861b EDID Timing Extension
//    Version3 and creates EELD (Enhanced EDID Like Data) packet. This EELD data contains
//    audio configuration information and other details read EDID.This can also contain Vendor specific Data
//
/////////////////////////////////////////////////////////////////////////
static int mdfld_hdmi_create_eeld_packet(struct drm_connector *connector)
{
    struct psb_intel_output *output = to_psb_intel_output(connector);
    struct mid_intel_hdmi_priv *hdmi_priv = output->dev_priv;
    uint8_t ucEdidBlock[128];
    hdmi_eeld_t *pEEld                = NULL;
    baseedid_1_x_t *pEdid        = NULL;
    ce_edid_t *pCeEdid        = NULL;
    int dwNumOfBytes        = 0;
    int sizeOfCEADataBlock    = 0;
    uint8_t * pDataBlock        = NULL;
    edid_dtd_timing_t *pDTD    = NULL;
    uint8_t *pData            = NULL;
    uint8_t ucDataBlockTag    = 0;
    cea_861b_adb_t *pADB        = NULL;
    uint8_t i                    = 0;
    uint8_t j                    = 0;
    uint8_t * pSADBlocks = NULL;
    uint8_t * pCurrentSADBlocks = NULL;
    uint32_t ulNumSADBytes = 0;
    //vsdb_byte6_to_byte8_t *pVSDB = NULL;
    uint32_t ulIndex = 0;
    //uint8_t b48kHzCADPresent = false;

    pEEld = (hdmi_eeld_t *) &hdmi_priv->eeld;

    // Fill Version info
    pEEld->cea_edid_rev_id = HDMI_EELD_CEA_EDID_VERSION;
    pEEld->eld_ver = HDMI_EELD_VERSION;

    // Fill BaseLine ELD length
    // This is 80 bytes as per EELD proposal
    pEEld->baseline_eld_length = HDMI_EELD_BASELINE_DATA_LENGTH;

    //Zero out EDID block buffer
    memset(ucEdidBlock, 0, sizeof(ucEdidBlock));

    // Get Extn EDID
    if(!mdfld_hdmi_get_cached_edid_block(connector, 1, ucEdidBlock, EDID_BLOCK_SIZE))
    {
        return 0;
    }    

    pCeEdid = (ce_edid_t *) ucEdidBlock;

    //allocate memory (48 bytes) for SAD Blocks buffer
    pSADBlocks = kcalloc(1, 48, GFP_KERNEL);

    if(pSADBlocks == NULL)
    {
	DRM_ERROR("mdfld_hdmi_create_eld_packaet() - Failed to allocate mem for pSADBlocks\n");
        return 0;
    }

    pCurrentSADBlocks = pSADBlocks;

    // Now pull out data from CEA Extension EDID
    // If Offset <= 4, we will not have CEA DataBlocks
    if(pCeEdid->ucDTDOffset > CEA_EDID_HEADER_SZIE)
    {
        sizeOfCEADataBlock = pCeEdid->ucDTDOffset - CEA_EDID_HEADER_SZIE;

        pDataBlock = (uint8_t *)pCeEdid;

        // skip header (first 4 bytes) in CEA EDID Timing Extension
        // and set pointer to start of DataBlocks collection
        pDataBlock += CEA_EDID_HEADER_SZIE;

        // General Format of CEA Data Block Collection
        // -----------+--------------------+-----------------------------------------+
        //            |Byte#   |bits5-7    |       bits 0-4                          |
        // -----------|--------------------+-----------------------------------------+
        //            |  1     | Video Tag |Length = total #of video bytes following |
        //            |        |    Code   |this byte (L1)                           |
        //            |--------------------+-----------------------------------------+
        //  Video     |  2     | CEA Short Video Descriptor 1                        |
        //  Data      |--------+-----------------------------------------------------|
        //  Block     |  3     | CEA Short Video Descriptor 2                        |
        //            |--------+-----------------------------------------------------|
        //            | ...    | ...                                                 |
        //            |--------------------------------------------------------------+
        //            | 1+L1   | CEA Short Video Descriptor L1                       |
        // -----------+--------------------+-----------------------------------------+
        //            | 2+L1   | Audio Tag |Length = total #of audio bytes following |
        //            |        |    Code   |this byte (L2)                           |
        //            |--------------------+-----------------------------------------+
        //  Audio     | 3+L1   |                                                     |
        //  Data      |--------+                                                     |
        //  Block     | 4+L1   | CEA Short Audio Descriptor 1                        |
        //            |--------+                                                     |
        //            | 5+L1   |                                                     |
        //            |--------------------------------------------------------------+
        //            | ...    |                                                     |
        //            |        |                                                     |
        //            |        |                                                     |
        //            | ...    |                                                     |
        //            |---------------------------------------------------------------
        //            |L1+L2   |                                                     |
        //            |--------|                                                     |
        //            |1+L1+L2 | CEA Short Audio Descriptor L2/3                     |
        //            |--------|                                                     |
        //            |2+L1+L2 |                                                     |
        // -----------+--------------------------------------------------------------+
        //            |3+L1+L2 |  Speaker  |Length = total #of SA bytes following    |
        //            |        | Tag Code  |this byte (L1)                           |
        //  Speaker   |--------------------------------------------------------------+
        //  Allocation|4+L1+L2 |                                                     |
        //  Data      |--------|                                                     |
        //  Block     |5+L1+L2 | Speaker Allocation Data Block Payload(3 bytes)      |
        //            |--------|                                                     |
        //            |6+L1+L2 |                                                     |
        // -----------+--------------------------------------------------------------+
        //            |7+L1+L2 | VSDB  Tag |Length = total #of VSDB bytes following  |
        //            |        |    Code   |this byte (L1)                           |
        //  Vendor    |--------------------------------------------------------------+
        //  Specific  |8+L1+L2 |                                                     |
        //  Data      |--------|                                                     |
        //  Block     |9+L1+L2 | 24-bit IEEE Registration Identifier (LSB first)     |
        //            |--------|                                                     |
        //            |10+L1+L2|                                                     |
        //            |--------------------------------------------------------------+
        //            | ...    | Vendor Specific Data block Payload                  |
        // -----------+--------------------------------------------------------------+

        while(sizeOfCEADataBlock > 0)
        {
            // Get the Size of CEA DataBlock in bytes and TAG
            dwNumOfBytes = *pDataBlock & CEA_DATABLOCK_LENGTH_MASK;
            ucDataBlockTag = (*pDataBlock & CEA_DATABLOCK_TAG_MASK) >> 5;

            switch(ucDataBlockTag)
            {
                case CEA_AUDIO_DATABLOCK:
                    // move beyond tag/length byte
                    ++pDataBlock;
                    for (i = 0; i < (dwNumOfBytes / 3); ++i, pDataBlock += 3)
                    {
                        pADB = (cea_861b_adb_t*)pDataBlock;
                        switch(pADB->audio_format_code)
                        {
                            // uncompressed audio (Linear PCM)
                            case AUDIO_LPCM:
                                memcpy(&(hdmi_priv->lpcm_sad),pDataBlock,3);
                                //save these blocks
                                memcpy(pCurrentSADBlocks, pDataBlock, 3);
                                // move pointer in SAD blocks buffer
                                pCurrentSADBlocks += 3;
                                // update SADC field
                                pEEld->sadc += 1;
                                break;
                            // compressed audio
                            case AUDIO_AC3:
                            case AUDIO_MPEG1:
                            case AUDIO_MP3:
                            case AUDIO_MPEG2:
                            case AUDIO_AAC:
                            case AUDIO_DTS:
                            case AUDIO_ATRAC:
                            case AUDIO_OBA:
                            case AUDIO_DOLBY_DIGITAL:
                            case AUDIO_DTS_HD:
                            case AUDIO_MAT:
                            case AUDIO_DST:
                            case AUDIO_WMA_PRO:
                                //save these blocks
                                memcpy(pCurrentSADBlocks, pDataBlock, 3);
                                // move pointer in SAD blocks buffer
                                pCurrentSADBlocks += 3;
                                // update SADC field
                                pEEld->sadc += 1;
                                break;
                        }
                    }
                    break;

                case CEA_VENDOR_DATABLOCK:
                    // audio wants data from 6th byte of VSDB onwards
                    //Sighting 94842: 

                    // | Byte # |    bits[7-0]                                              |
                    // |--------------------------------------------------------------------|
                    // | 1-3    |24-bit IEEE Registration Identifier (0x000C03)             |
                    // |--------------------------------------------------------------------|
                    // | 4-5    |       Source Physical Address                             |
                    // |--------------------------------------------------------------------|
                    // | 6      |SupportsAI|DC48bit|DC36bit|Dc30bit|DCY444|Rsvd|Rsvd|DVIDual|
                    // |--------------------------------------------------------------------|
                    // | 7      |   Max TMDS clock                                          |
                    // |--------------------------------------------------------------------|
                    // | 8      |Latency_Field |I_Latency_Field| Reserved bits 5-0          |
                    // |        |   _Present   |  _Present     |                            |
                    // |--------------------------------------------------------------------|
                    // | 9      |               Video Latency                               |
                    // |--------------------------------------------------------------------|
                    // | 10     |               Audio Latency                               |
                    // |--------------------------------------------------------------------|
                    // | 11     |            Interlaced Video Latency                       |
                    // |--------------------------------------------------------------------|
                    // | 12     |            Interlaced Audio Latency                       |
                    // |--------------------------------------------------------------------|

                    ++pDataBlock;
                    // move pointer to next CEA Datablock
                    pDataBlock += dwNumOfBytes;
                    break;                 

                case CEA_SPEAKER_DATABLOCK:
                    pEEld->speaker_allocation_block = *(++pDataBlock);
                    // move pointer to next CEA Datablock
                    pDataBlock += dwNumOfBytes;
                    break;                       

                default:
                    // Move pointer to next CEA DataBlock
                    pDataBlock += (dwNumOfBytes + 1);
            }
            // Decrement size of CEA DataBlock
            sizeOfCEADataBlock -= (dwNumOfBytes + 1);
        }    
    }

    //Copy all the saved SAD blocks at the end of ELD
    //SAD blocks should be written after the Monitor name and VSDB.
    //See ELD definition in iHDMI.h
    ulNumSADBytes = (pEEld->sadc) * 3; //Size of each SAD block is 3 bytes

    //DCN 460119: Audio does not play on displays which do not provide SAB in EDID.
    //Solution: Graphics driver should create a default SAB in ELD with front left and front right
    //speakers enabled if the display supports basic audio. 
    pDataBlock = (uint8_t *)pCeEdid;
    if((*(pDataBlock + HDMI_CEA_EXTENSION_BLOCK_BYTE_3) & HDMI_BASIC_AUDIO_SUPPORTED) && (pEEld->speaker_allocation_block == 0)) 
    {
        pEEld->flr = 1;
    }
    //End of DCN 460119

    // zero out local buffers
    memset(ucEdidBlock, 0, sizeof(ucEdidBlock));

    // Get base EDID
    if(!mdfld_hdmi_get_cached_edid_block(connector, 0, ucEdidBlock, EDID_BLOCK_SIZE))
    {
        return 0;
    }

    pEdid = (baseedid_1_x_t*) ucEdidBlock;
    pDTD = &pEdid->DTD[1];

    //Update the Manufacturer ID and Product Code here
    memcpy(pEEld->manufacturer_id,pEdid->ManufacturerID,2);
    memcpy(pEEld->product_id,pEdid->ProductID,2);

    // Now Fill the monitor string name
    // Search through DTD blocks, looking for monitor name
    for (i = 0; i < MAX_BASEEDID_DTD_BLOCKS - 1; ++i, ++pDTD)
    {
        // Set a uint8_t pointer to DTD data
        pData = (uint8_t *)pDTD;

        // Check the Flag (the first two bytes) to determine
        // if this block is used as descriptor
        if (pData[0] == 0x00 && pData[1] == 0x00)
        {
            // And now check Data Type Tag within this descriptor
            // Tag = 0xFC, then monitor name stored as ASCII
            if (pData[3] == 0xFC)
            {
                ulIndex = 0;
                // Copy monitor name
                for (j = 0; (j < 13) && (pData[j+5] != 0x0A); ++j)
                {
                    pEEld->mn_sand_sads[ulIndex] = pData[j+5];
                    ulIndex++;
                }
                pEEld->mnl = j;
                break;
            }
        }
    }

    //Check if number of SAD Bytes > 0 and for size within limits of allowed Base line Data size as per EELD spec
    if((ulNumSADBytes > 0) && (ulNumSADBytes <= 64))
    {
        //Copy the SADs immediately after the Monitor Name String
        memcpy(&pEEld->mn_sand_sads[j], pSADBlocks, ulNumSADBytes);
    }


    // Header = 4, Baseline Data = 60 and Vendor (INTEL) specific = 2
    // 4 + 60 + 2 = 66
    hdmi_priv->hdmi_eeld_size = HDMI_EELD_SIZE;
    
    //free the buffer allocated for SAD blocks
    kfree(pSADBlocks);
    pSADBlocks = NULL;
    pCurrentSADBlocks = NULL;
    return 1;
}

#endif

static enum drm_connector_status mdfld_hdmi_detect(struct drm_connector *connector, bool force)
{
	struct psb_intel_output *psb_intel_output = to_psb_intel_output(connector);
	struct mid_intel_hdmi_priv *hdmi_priv = psb_intel_output->dev_priv;
	struct edid *edid = NULL;
	enum drm_connector_status status = connector_status_disconnected;

	PSB_DEBUG_ENTRY("\n");

	edid = drm_get_edid(&psb_intel_output->base,
			 psb_intel_output->hdmi_i2c_adapter);

	hdmi_priv->has_hdmi_sink = false;
	hdmi_priv->has_hdmi_audio = false;
	if (edid) {
		if (edid->input & DRM_EDID_INPUT_DIGITAL) {
			status = connector_status_connected;
			hdmi_priv->has_hdmi_sink = drm_detect_hdmi_monitor(edid);
			hdmi_priv->has_hdmi_audio = drm_detect_monitor_audio(edid);
		}

		psb_intel_output->base.display_info.raw_edid = NULL;
		kfree(edid);
	}

	return status;
}

static int mdfld_hdmi_set_property(struct drm_connector *connector,
				       struct drm_property *property,
				       uint64_t value)
{
	struct drm_encoder *pEncoder = connector->encoder;

	PSB_DEBUG_ENTRY("connector info, type = %d, type_id=%d, base=0x%p, base.id=0x%x. \n", connector->connector_type, connector->connector_type_id, &connector->base, connector->base.id);
	PSB_DEBUG_ENTRY("encoder info, base.id=%d, encoder_type=%d, dev=0x%p, base=0x%p, possible_clones=0x%x. \n", pEncoder->base.id, pEncoder->encoder_type, pEncoder->dev, &pEncoder->base, pEncoder->possible_clones);
	PSB_DEBUG_ENTRY("encoder info, possible_crtcs=0x%x, crtc=0x%p.  \n", pEncoder->possible_crtcs, pEncoder->crtc);

	if (!strcmp(property->name, "scaling mode") && pEncoder) {
		PSB_DEBUG_ENTRY("scaling mode \n");
	} else if (!strcmp(property->name, "backlight") && pEncoder) {
		PSB_DEBUG_ENTRY("backlight \n");
	} else if (!strcmp(property->name, "DPMS") && pEncoder) {
		PSB_DEBUG_ENTRY("DPMS \n");
	}

	if (!strcmp(property->name, "scaling mode") && pEncoder) {
		struct psb_intel_crtc *pPsbCrtc = to_psb_intel_crtc(pEncoder->crtc);
		bool bTransitionFromToCentered;
		uint64_t curValue;

		if (!pPsbCrtc)
			goto set_prop_error;

		switch (value) {
		case DRM_MODE_SCALE_FULLSCREEN:
			break;
		case DRM_MODE_SCALE_NO_SCALE:
			break;
		case DRM_MODE_SCALE_ASPECT:
			break;
		default:
			goto set_prop_error;
		}

		if (drm_connector_property_get_value(connector, property, &curValue))
			goto set_prop_error;

		if (curValue == value)
			goto set_prop_done;

		if (drm_connector_property_set_value(connector, property, value))
			goto set_prop_error;

		bTransitionFromToCentered = (curValue == DRM_MODE_SCALE_NO_SCALE) ||
			(value == DRM_MODE_SCALE_NO_SCALE);

		if (pPsbCrtc->saved_mode.hdisplay != 0 &&
		    pPsbCrtc->saved_mode.vdisplay != 0) {
			if (bTransitionFromToCentered) {
				if (!drm_crtc_helper_set_mode(pEncoder->crtc, &pPsbCrtc->saved_mode,
					    pEncoder->crtc->x, pEncoder->crtc->y, pEncoder->crtc->fb))
					goto set_prop_error;
			} else {
				struct drm_encoder_helper_funcs *pEncHFuncs  = pEncoder->helper_private;
				pEncHFuncs->mode_set(pEncoder, &pPsbCrtc->saved_mode,
						     &pPsbCrtc->saved_adjusted_mode);
			}
		}
	}
set_prop_done:
    return 0;
set_prop_error:
    return -1;
}

/**
 * Return the list of HDMI DDC modes if available.
 */
static int mdfld_hdmi_get_modes(struct drm_connector *connector)
{
	struct psb_intel_output *psb_intel_output = to_psb_intel_output(connector);
	struct edid *edid = NULL;
	int ret = 0;

	PSB_DEBUG_ENTRY("\n");

	edid = drm_get_edid(&psb_intel_output->base,
			 psb_intel_output->hdmi_i2c_adapter);
	if (edid) {
		drm_mode_connector_update_edid_property(&psb_intel_output->
							base, edid);
		ret = drm_add_edid_modes(&psb_intel_output->base, edid);
		kfree(edid);
	}

	return ret;
}

static int mdfld_hdmi_mode_valid(struct drm_connector *connector,
				 struct drm_display_mode *mode)
{

	PSB_DEBUG_ENTRY("display info. hdisplay = %d, vdisplay = %d. \n", mode->hdisplay, mode->vdisplay);

	if (mode->clock > 165000)
		return MODE_CLOCK_HIGH;
	if (mode->clock < 20000)
		return MODE_CLOCK_HIGH;

	/* just in case */
	if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
		return MODE_NO_DBLESCAN;

	/* just in case */
	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		return MODE_NO_INTERLACE;

	return MODE_OK;
}

static const struct drm_encoder_helper_funcs mdfld_hdmi_helper_funcs = {
	.dpms = mdfld_hdmi_dpms,
	.mode_fixup = mdfld_hdmi_mode_fixup,
	.prepare = psb_intel_encoder_prepare,
	.mode_set = mdfld_hdmi_mode_set,
	.commit = psb_intel_encoder_commit,
};

static const struct drm_connector_helper_funcs mdfld_hdmi_connector_helper_funcs = {
	.get_modes = mdfld_hdmi_get_modes,
	.mode_valid = mdfld_hdmi_mode_valid,
	.best_encoder = psb_intel_best_encoder,
};

static const struct drm_connector_funcs mdfld_hdmi_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.save = mdfld_hdmi_save,
	.restore = mdfld_hdmi_restore,
	.detect = mdfld_hdmi_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.set_property = mdfld_hdmi_set_property,
	.destroy = psb_intel_lvds_destroy,
};

void mdfld_hdmi_init(struct drm_device *dev, struct psb_intel_mode_device *mode_dev, int reg)
{
	struct psb_intel_output *psb_intel_output;
	struct drm_connector *connector;
	struct drm_encoder *encoder;
	struct mid_intel_hdmi_priv *hdmi_priv;
	int ddc_bus;

	PSB_DEBUG_ENTRY("\n");

	psb_intel_output = kzalloc(sizeof(struct psb_intel_output) +
			       sizeof(struct mid_intel_hdmi_priv), GFP_KERNEL);
	if (!psb_intel_output)
		return;

	hdmi_priv = (struct mid_intel_hdmi_priv *)(psb_intel_output + 1);
	psb_intel_output->mode_dev = mode_dev;
	connector = &psb_intel_output->base;
	encoder = &psb_intel_output->enc;
	drm_connector_init(dev, &psb_intel_output->base,
			   &mdfld_hdmi_connector_funcs,
			   DRM_MODE_CONNECTOR_DVID);

	drm_encoder_init(dev, &psb_intel_output->enc, &psb_intel_lvds_enc_funcs,
			 DRM_MODE_ENCODER_TMDS);

	drm_mode_connector_attach_encoder(&psb_intel_output->base,
					  &psb_intel_output->enc);
	psb_intel_output->type = INTEL_OUTPUT_HDMI;
	hdmi_priv->hdmi_reg = reg;
	hdmi_priv->has_hdmi_sink = false;
	psb_intel_output->dev_priv = hdmi_priv;

	drm_encoder_helper_add(encoder, &mdfld_hdmi_helper_funcs);
	drm_connector_helper_add(connector,
				 &mdfld_hdmi_connector_helper_funcs);
	connector->display_info.subpixel_order = SubPixelHorizontalRGB;
	connector->interlace_allowed = false;
	connector->doublescan_allowed = false;

	connector->polled = DRM_CONNECTOR_POLL_HPD;

	drm_connector_attach_property(connector, dev->mode_config.scaling_mode_property, DRM_MODE_SCALE_FULLSCREEN);

	switch (reg) {
	case SDVOB:
		ddc_bus = GPIOE;
		psb_intel_output->ddi_select = DDI0_SELECT;
		break;
	case SDVOC:
		ddc_bus = GPIOD;
		psb_intel_output->ddi_select = DDI1_SELECT;
		break;
	default:
		DRM_ERROR("unknown reg 0x%x for HDMI\n", reg);
		goto failed_ddc;
		break;
	}

	psb_intel_output->ddc_bus = psb_intel_i2c_create(dev,
						ddc_bus, (reg == SDVOB) ? "HDMIB":"HDMIC");

	if (!psb_intel_output->ddc_bus) {
		DRM_ERROR("No ddc adapter available!\n");
		goto failed_ddc;
	}
	psb_intel_output->hdmi_i2c_adapter = &(psb_intel_output->ddc_bus->adapter);

	hdmi_priv->is_hdcp_supported = true;
	hdmi_priv->dev = dev; 
	drm_sysfs_connector_add(connector);
	return;

failed_ddc:
	drm_encoder_cleanup(&psb_intel_output->enc);
	drm_connector_cleanup(&psb_intel_output->base);
	kfree(psb_intel_output);
}
