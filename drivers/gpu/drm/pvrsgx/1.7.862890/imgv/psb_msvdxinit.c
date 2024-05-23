/**************************************************************************
 * psb_msvdxinit.c
 * MSVDX initialization and mtx-firmware upload
 *
 * Copyright (c) 2011 Intel Corporation, Hillsboro, OR, USA
 * Copyright (c) Imagination Technologies Limited, UK
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 **************************************************************************/

#include <drm/drmP.h>
#include <drm/drm.h>
#include "psb_drv.h"
#include "psb_msvdx.h"
#include <linux/firmware.h>

#define MSVDX_REG (dev_priv->msvdx_reg)
#define UPLOAD_FW_BY_DMA 1
#define STACKGUARDWORD          ( 0x10101010 )
#define MSVDX_MTX_DATA_LOCATION ( 0x82880000 )
#define UNINITILISE_MEM 	( 0xcdcdcdcd )
#define FIRMWAREID		( 0x014d42ab )

uint8_t psb_rev_id;
/*MSVDX FW header*/
struct msvdx_fw {
	uint32_t ver;
	uint32_t text_size;
	uint32_t data_size;
	uint32_t data_location;
};

int psb_wait_for_register(struct drm_psb_private *dev_priv,
			  uint32_t offset, uint32_t value, uint32_t enable,
				uint32_t loops, uint32_t interval)
{
	uint32_t reg_value = 0;
	uint32_t poll_cnt = loops;
	while (poll_cnt) {
		reg_value = PSB_RMSVDX32(offset);
		if (value == (reg_value & enable))	/* All the bits are reset   */
			return 0;	/* So exit			*/

		/* Wait a bit */
		DRM_UDELAY(interval);
		poll_cnt--;
	}
	DRM_ERROR("MSVDX: Timeout while waiting for register %08x:"
		  " expecting %08x (mask %08x), got %08x\n",
		  offset, value, enable, reg_value);

	return 1;
}

int psb_poll_mtx_irq(struct drm_psb_private *dev_priv)
{
	int ret = 0;
	uint32_t mtx_int = 0;

	REGIO_WRITE_FIELD_LITE(mtx_int, MSVDX_INTERRUPT_STATUS, CR_MTX_IRQ,
			       1);

	ret = psb_wait_for_register(dev_priv, MSVDX_INTERRUPT_STATUS,
				    /* Required value */
				    mtx_int,
				    /* Enabled bits */
				    mtx_int, 10000, 100);

	if (ret) {
		DRM_ERROR("MSVDX: Error Mtx did not return"
			  " int within a resonable time\n");
		return ret;
	}

	PSB_DEBUG_IRQ("MSVDX: Got MTX Int\n");

	/* Got it so clear the bit */
	PSB_WMSVDX32(mtx_int, MSVDX_INTERRUPT_CLEAR);

	return ret;
}

void psb_write_mtx_core_reg(struct drm_psb_private *dev_priv,
			    const uint32_t core_reg, const uint32_t val)
{
	uint32_t reg = 0;

	/* Put data in MTX_RW_DATA */
	PSB_WMSVDX32(val, MSVDX_MTX_REGISTER_READ_WRITE_DATA);

	/* DREADY is set to 0 and request a write */
	reg = core_reg;
	REGIO_WRITE_FIELD_LITE(reg, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST,
			       MTX_RNW, 0);
	REGIO_WRITE_FIELD_LITE(reg, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST,
			       MTX_DREADY, 0);
	PSB_WMSVDX32(reg, MSVDX_MTX_REGISTER_READ_WRITE_REQUEST);

	psb_wait_for_register(dev_priv,
			      MSVDX_MTX_REGISTER_READ_WRITE_REQUEST,
			      MSVDX_MTX_REGISTER_READ_WRITE_REQUEST_MTX_DREADY_MASK,
			      MSVDX_MTX_REGISTER_READ_WRITE_REQUEST_MTX_DREADY_MASK,
				100, 100);
}

#if UPLOAD_FW_BY_DMA

static void psb_get_mtx_control_from_dash(struct drm_psb_private *dev_priv)
{
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;
	int count = 0;
	uint32_t reg_val = 0;

	REGIO_WRITE_FIELD(reg_val, MSVDX_MTX_DEBUG, MTX_DBG_IS_SLAVE, 1);
	REGIO_WRITE_FIELD(reg_val, MSVDX_MTX_DEBUG, MTX_DBG_GPIO_IN, 0x02);
	PSB_WMSVDX32(reg_val, MSVDX_MTX_DEBUG);

	do
	{
		reg_val = PSB_RMSVDX32(MSVDX_MTX_DEBUG);
		count++;
	} while (((reg_val & 0x18) != 0) && count < 50000);

	if(count >= 50000)
		PSB_DEBUG_GENERAL("TOPAZ: timeout in get_mtx_control_from_dash\n"); 

	/* Save the access control register...*/
	msvdx_priv->psb_dash_access_ctrl = PSB_RMSVDX32(MSVDX_MTX_RAM_ACCESS_CONTROL);
		
}

static void psb_release_mtx_control_from_dash(struct drm_psb_private *dev_priv)
{
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;

	/* restore access control */
	PSB_WMSVDX32(msvdx_priv->psb_dash_access_ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);
	/* release bus */
	PSB_WMSVDX32(0x4, MSVDX_MTX_DEBUG);
}



static void psb_upload_fw(struct drm_psb_private *dev_priv,
			  uint32_t address, const unsigned int words, int fw_sel)
{
	uint32_t reg_val=0;
	uint32_t cmd;
	uint32_t uCountReg, offset, mmu_ptd;
	uint32_t size = (words*4 ); /* byte count */
	uint32_t dma_channel = 0; /* Setup a Simple DMA for Ch0 */
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;

	PSB_DEBUG_GENERAL("MSVDX: Upload firmware by DMA\n");
	psb_get_mtx_control_from_dash(dev_priv);

	// dma transfers to/from the mtx have to be 32-bit aligned and in multiples of 32 bits
	PSB_WMSVDX32(address, REGISTER(MTX_CORE, CR_MTX_SYSC_CDMAA));

	REGIO_WRITE_FIELD_LITE(reg_val, MTX_CORE_CR_MTX_SYSC_CDMAC, BURSTSIZE,	4 );// burst size in multiples of 64 bits (allowed values are 2 or 4)
	REGIO_WRITE_FIELD_LITE(reg_val, MTX_CORE_CR_MTX_SYSC_CDMAC, RNW, 0);	// false means write to mtx mem, true means read from mtx mem
	REGIO_WRITE_FIELD_LITE(reg_val, MTX_CORE_CR_MTX_SYSC_CDMAC, ENABLE,	1);				// begin transfer
	REGIO_WRITE_FIELD_LITE(reg_val, MTX_CORE_CR_MTX_SYSC_CDMAC, LENGTH,	words  );		// This specifies the transfer size of the DMA operation in terms of 32-bit words
	PSB_WMSVDX32(reg_val, REGISTER(MTX_CORE, CR_MTX_SYSC_CDMAC));

	// toggle channel 0 usage between mtx and other msvdx peripherals
	{
		reg_val = PSB_RMSVDX32(REGISTER(  MSVDX_CORE, CR_MSVDX_CONTROL));
		REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_MSVDX_CONTROL, DMAC_CH0_SELECT,  0 );
		PSB_WMSVDX32(reg_val, REGISTER( MSVDX_CORE, CR_MSVDX_CONTROL));
	}


	/* Clear the DMAC Stats */
	PSB_WMSVDX32(0 , REGISTER(DMAC, DMAC_IRQ_STAT ) + dma_channel);

	offset = msvdx_priv->fw->offset;

	if(fw_sel)
		offset += ((msvdx_priv->mtx_mem_size + 8192) & ~0xfff);

	/* use bank 0 */
	cmd = 0;
	PSB_WMSVDX32(cmd, REGISTER(MSVDX_CORE, CR_MMU_BANK_INDEX));

	/* Write PTD to mmu base 0*/
	mmu_ptd = psb_get_default_pd_addr(dev_priv->mmu);
	PSB_WMSVDX32(mmu_ptd, REGISTER( MSVDX_CORE, CR_MMU_DIR_LIST_BASE) + 0);

	/* Invalidate */
	reg_val = PSB_RMSVDX32(REGISTER(MSVDX_CORE, CR_MMU_CONTROL0));
	reg_val &= ~0xf;
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_MMU_CONTROL0, CR_MMU_INVALDC, 1 );
	PSB_WMSVDX32(reg_val, REGISTER(MSVDX_CORE, CR_MMU_CONTROL0 ));

	PSB_WMSVDX32(offset, REGISTER(DMAC, DMAC_SETUP ) + dma_channel);

	/* Only use a single dma - assert that this is valid */
	if( (size / 4 ) >= (1<<15) ) {
		DRM_ERROR("psb: DMA size beyond limited, aboart firmware uploading\n");
		return;
	}
		

	uCountReg = PSB_DMAC_VALUE_COUNT(PSB_DMAC_BSWAP_NO_SWAP,
					 0,  /* 32 bits */
					 PSB_DMAC_DIR_MEM_TO_PERIPH,
					 0,
					 (size / 4 ) );
	/* Set the number of bytes to dma*/
	PSB_WMSVDX32(uCountReg, REGISTER(DMAC, 	DMAC_COUNT ) + dma_channel);

	cmd = PSB_DMAC_VALUE_PERIPH_PARAM(PSB_DMAC_ACC_DEL_0, PSB_DMAC_INCR_OFF, PSB_DMAC_BURST_2);
	PSB_WMSVDX32(cmd, REGISTER(DMAC, DMAC_PERIPH ) + dma_channel);

	/* Set destination port for dma */
	cmd = 0;
	REGIO_WRITE_FIELD(cmd, DMAC_DMAC_PERIPHERAL_ADDR, ADDR, MTX_CORE_CR_MTX_SYSC_CDMAT_OFFSET); 
	PSB_WMSVDX32(cmd, REGISTER(DMAC, DMAC_PERIPHERAL_ADDR ) + dma_channel);


	/* Finally, rewrite the count register with the enable bit set*/
	PSB_WMSVDX32(uCountReg | DMAC_DMAC_COUNT_EN_MASK, REGISTER(DMAC, DMAC_COUNT ) + dma_channel);

	/* Wait for all to be done */
	if(psb_wait_for_register(dev_priv, 
				 REGISTER(DMAC, DMAC_IRQ_STAT ) + dma_channel, 
				 DMAC_DMAC_IRQ_STAT_TRANSFER_FIN_MASK, 
				 DMAC_DMAC_IRQ_STAT_TRANSFER_FIN_MASK, 10000,100)) {
		psb_release_mtx_control_from_dash(dev_priv);
		return;
	}

	/* Assert that the MTX DMA port is all done aswell */
	if(psb_wait_for_register(dev_priv, REGISTER(MTX_CORE, CR_MTX_SYSC_CDMAS0), 1, 1, 10, 10)) {
		psb_release_mtx_control_from_dash(dev_priv);
		return;
	}

	psb_release_mtx_control_from_dash(dev_priv);
	PSB_DEBUG_GENERAL("MSVDX: Upload done\n");
}

#else

static void psb_upload_fw(struct drm_psb_private *dev_priv,
			  const uint32_t data_mem, uint32_t ram_bank_size,
			  uint32_t address, const unsigned int words,
			  const uint32_t * const data)
{
	uint32_t loop, ctrl, ram_id, addr, cur_bank = (uint32_t) ~0;
	uint32_t access_ctrl;

	PSB_DEBUG_GENERAL("MSVDX: Upload firmware by register interface\n");
	/* Save the access control register... */
	access_ctrl = PSB_RMSVDX32(MSVDX_MTX_RAM_ACCESS_CONTROL);

	/* Wait for MCMSTAT to become be idle 1 */
	psb_wait_for_register(dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS,
			      1,	/* Required Value */
			      0xffffffff,/* Enables */
				1000,100);

	for (loop = 0; loop < words; loop++) {
		ram_id = data_mem + (address / ram_bank_size);
		if (ram_id != cur_bank) {
			addr = address >> 2;
			ctrl = 0;
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMID, ram_id);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCM_ADDR, addr);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMAI, 1);
			PSB_WMSVDX32(ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);
			cur_bank = ram_id;
		}
		address += 4;

		PSB_WMSVDX32(data[loop],
			     MSVDX_MTX_RAM_ACCESS_DATA_TRANSFER);

		/* Wait for MCMSTAT to become be idle 1 */
		psb_wait_for_register(dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS,
				      1,	/* Required Value */
				      0xffffffff, /* Enables */
					100, 100);
	}
	PSB_DEBUG_GENERAL("MSVDX: Upload done\n");

	/* Restore the access control register... */
	PSB_WMSVDX32(access_ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);
}

#endif

static int psb_verify_fw(struct drm_psb_private *dev_priv,
			 const uint32_t ram_bank_size,
			 const uint32_t data_mem, uint32_t address,
			 const uint32_t words, const uint32_t * const data)
{
	uint32_t loop, ctrl, ram_id, addr, cur_bank = (uint32_t) ~0;
	uint32_t access_ctrl;
	int ret = 0;

	/* Save the access control register... */
	access_ctrl = PSB_RMSVDX32(MSVDX_MTX_RAM_ACCESS_CONTROL);

	/* Wait for MCMSTAT to become be idle 1 */
	psb_wait_for_register(dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS,
			      1,	/* Required Value */
			      0xffffffff, /* Enables */
				100, 100);

	for (loop = 0; loop < words; loop++) {
		uint32_t reg_value;
		ram_id = data_mem + (address / ram_bank_size);

		if (ram_id != cur_bank) {
			addr = address >> 2;
			ctrl = 0;
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMID, ram_id);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCM_ADDR, addr);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMAI, 1);
			REGIO_WRITE_FIELD_LITE(ctrl,
					       MSVDX_MTX_RAM_ACCESS_CONTROL,
					       MTX_MCMR, 1);

			PSB_WMSVDX32(ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);

			cur_bank = ram_id;
		}
		address += 4;

		/* Wait for MCMSTAT to become be idle 1 */
		psb_wait_for_register(dev_priv, MSVDX_MTX_RAM_ACCESS_STATUS,
				      1,	/* Required Value */
				      0xffffffff, /* Enables */
					100, 100);

		reg_value = PSB_RMSVDX32(MSVDX_MTX_RAM_ACCESS_DATA_TRANSFER);
		if (data[loop] != reg_value) {
			DRM_ERROR("psb: Firmware validation fails"
				  " at index=%08x\n", loop);
			ret = 1;
			break;
		}
	}

	/* Restore the access control register... */
	PSB_WMSVDX32(access_ctrl, MSVDX_MTX_RAM_ACCESS_CONTROL);

	return ret;
}

static int msvdx_get_fw_bo(struct drm_device *dev,
			   const struct firmware **raw, uint8_t *name)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	int rc, fw_size;
	void *ptr = NULL;
        struct ttm_bo_kmap_obj tmp_kmap;
        bool is_iomem;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;
	void *gpu_addr;

	rc = request_firmware(raw, name, &dev->pdev->dev);
	if (rc < 0) {
		DRM_ERROR("MSVDX: %s request_firmware failed: Reason %d\n",
			  name, rc);
		return 1;
	}

	if ((*raw)->size < sizeof(struct msvdx_fw)) {
		DRM_ERROR("MSVDX: %s is is not correct size(%zd)\n",
			  name, (*raw)->size);
		return 1;
	}

	ptr = (void *) ((*raw))->data;

	if (!ptr) {
		DRM_ERROR("MSVDX: Failed to load %s\n", name);
		return 1;
	}

	/* another sanity check... */
	fw_size = sizeof(struct msvdx_fw) +
		sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->text_size +
		sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->data_size;
	if ((*raw)->size < fw_size) {
		DRM_ERROR("MSVDX: %s is is not correct size(%zd)\n",
			  name, (*raw)->size);
		return 1;
	}

	rc = ttm_bo_kmap(msvdx_priv->fw, 0, (msvdx_priv->fw)->num_pages, &tmp_kmap);
	if (rc) {
		PSB_DEBUG_GENERAL("drm_bo_kmap failed: %d\n", rc);
		ttm_bo_unref(&msvdx_priv->fw);
		ttm_bo_kunmap(&tmp_kmap);
		return 1;
	}		
	else {
		uint32_t *last_word;
		gpu_addr = ttm_kmap_obj_virtual(&tmp_kmap, &is_iomem);

		memset(gpu_addr, UNINITILISE_MEM, msvdx_priv->mtx_mem_size);

		memcpy(gpu_addr, ptr + sizeof(struct msvdx_fw),
			sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->text_size);

		memcpy(gpu_addr + (((struct msvdx_fw *) ptr)->data_location - MSVDX_MTX_DATA_LOCATION),
			(void *)ptr + sizeof(struct msvdx_fw) + sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->text_size, 
			sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->data_size);

		last_word = (uint32_t *) (gpu_addr + msvdx_priv->mtx_mem_size -4);
		/* Write a know value to last word in mtx memory*/
		/* Usefull for detection of stack overrun */
		*last_word = STACKGUARDWORD;
	}

	ttm_bo_kunmap(&tmp_kmap);
	PSB_DEBUG_GENERAL("MSVDX: releasing firmware resouces\n");
	PSB_DEBUG_GENERAL("MSVDX: Load firmware into BO successfully\n");
	release_firmware(*raw);
	return rc;
}


static uint32_t *msvdx_get_fw(struct drm_device *dev,
			      const struct firmware **raw, uint8_t *name)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	int rc, fw_size;
	void *ptr = NULL;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;

	rc = request_firmware(raw, name, &dev->pdev->dev);
	if (rc < 0) {
		DRM_ERROR("MSVDX: %s request_firmware failed: Reason %d\n",
			  name, rc);
		return NULL;
	}

	if ((*raw)->size < sizeof(struct msvdx_fw)) {
		DRM_ERROR("MSVDX: %s is is not correct size(%zd)\n",
			  name, (*raw)->size);
		return NULL;
	}

	ptr = (int *) ((*raw))->data;

	if (!ptr) {
		DRM_ERROR("MSVDX: Failed to load %s\n", name);
		return NULL;
	}

	/* another sanity check... */
	fw_size = sizeof(struct msvdx_fw) +
		sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->text_size +
		sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->data_size;
	if ((*raw)->size < fw_size) {
		DRM_ERROR("MSVDX: %s is is not correct size(%zd)\n",
			  name, (*raw)->size);
		return NULL;
	}
	else if((*raw)->size > fw_size) { /* there is ec firmware */
		ptr += ((fw_size + 0xfff) & ~0xfff);
		fw_size += (sizeof(struct msvdx_fw) +
			    sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->text_size +
			    sizeof(uint32_t) * ((struct msvdx_fw *) ptr)->data_size);

		ptr = (int *) ((*raw))->data; /* Resotre ptr to start of the firmware file */
	}

	msvdx_priv->msvdx_fw = kzalloc(fw_size, GFP_KERNEL);
	if (msvdx_priv->msvdx_fw == NULL)
		DRM_ERROR("MSVDX: allocate FW buffer failed\n");
	else {
		memcpy(msvdx_priv->msvdx_fw, ptr, fw_size);
		msvdx_priv->msvdx_fw_size = fw_size;
	}

	PSB_DEBUG_GENERAL("MSVDX: releasing firmware resouces\n");
	release_firmware(*raw);

	return msvdx_priv->msvdx_fw;
}

int psb_setup_fw(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	uint32_t ram_bank_size;
	struct msvdx_fw *fw;
	uint32_t *fw_ptr = NULL;
	uint32_t *text_ptr = NULL;
	uint32_t *data_ptr = NULL;
	const struct firmware *raw = NULL;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;
	int ec_firmware = 0, ret = 0;

	/* todo : Assert the clock is on - if not turn it on to upload code */
	PSB_DEBUG_GENERAL("MSVDX: psb_setup_fw\n");
	PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);

	/* Reset MTX */
	PSB_WMSVDX32(MSVDX_MTX_SOFT_RESET_MTX_RESET_MASK,
		     MSVDX_MTX_SOFT_RESET);

	/* Initialses Communication controll area to 0 */
/*
  if (psb_rev_id >= POULSBO_D1) {
  PSB_DEBUG_GENERAL("MSVDX: Detected Poulsbo D1"
  " or later revision.\n");
  PSB_WMSVDX32(MSVDX_DEVICE_NODE_FLAGS_DEFAULT_D1,
  MSVDX_COMMS_OFFSET_FLAGS);
  } else {
  PSB_DEBUG_GENERAL("MSVDX: Detected Poulsbo D0"
  " or earlier revision.\n");
  PSB_WMSVDX32(MSVDX_DEVICE_NODE_FLAGS_DEFAULT_D0,
  MSVDX_COMMS_OFFSET_FLAGS);
  }
*/

	if(IS_CDV(dev)) {
		PSB_WMSVDX32(FIRMWAREID, MSVDX_COMMS_FIRMWARE_ID);
	}

	PSB_WMSVDX32(0, MSVDX_COMMS_ERROR_TRIG);
	PSB_WMSVDX32(199, MSVDX_MTX_SYSC_TIMERDIV); /* MTX_SYSC_TIMERDIV */
	PSB_WMSVDX32(0, MSVDX_EXT_FW_ERROR_STATE); /* EXT_FW_ERROR_STATE */

	PSB_WMSVDX32(0, MSVDX_COMMS_MSG_COUNTER);
	PSB_WMSVDX32(0, MSVDX_COMMS_SIGNATURE);
	PSB_WMSVDX32(0, MSVDX_COMMS_TO_HOST_RD_INDEX);
	PSB_WMSVDX32(0, MSVDX_COMMS_TO_HOST_WRT_INDEX);
	PSB_WMSVDX32(0, MSVDX_COMMS_TO_MTX_RD_INDEX);
	PSB_WMSVDX32(0, MSVDX_COMMS_TO_MTX_WRT_INDEX);
	PSB_WMSVDX32(0, MSVDX_COMMS_FW_STATUS);
	PSB_WMSVDX32(DSIABLE_IDLE_GPIO_SIG | DSIABLE_Auto_CLOCK_GATING | RETURN_VDEB_DATA_IN_COMPLETION | DSIABLE_FW_WDT,
		     MSVDX_COMMS_OFFSET_FLAGS);
	PSB_WMSVDX32(0, MSVDX_COMMS_SIGNATURE);
	/* read register bank size */
	{
		uint32_t bank_size, reg;
		reg = PSB_RMSVDX32(MSVDX_MTX_RAM_BANK);
		bank_size =
			REGIO_READ_FIELD(reg, MSVDX_MTX_RAM_BANK,
					 CR_MTX_RAM_BANK_SIZE);
		ram_bank_size = (uint32_t) (1 << (bank_size + 2));
	}

	PSB_DEBUG_GENERAL("MSVDX: RAM bank size = %d bytes\n",
			  ram_bank_size);

	/* if FW already loaded from storage */
	if (msvdx_priv->msvdx_fw)
		fw_ptr = msvdx_priv->msvdx_fw;
	else {
		if (IS_CDV(dev)) {
			if(IS_FW_UPDATED)
				fw_ptr = msvdx_get_fw(dev, &raw, "msvdx_fw_mfld_DE2.0.bin");
			else
				fw_ptr = msvdx_get_fw(dev, &raw, "msvdx_fw_mfld.bin");

			PSB_DEBUG_GENERAL("MSVDX:load msvdx_fw_mfld_DE2.0.bin by udevd\n");
		}
		else
			DRM_ERROR("MSVDX:HW is neither mrst nor mfld\n");
	}

	if (!fw_ptr) {
		DRM_ERROR("MSVDX:load msvdx_fw.bin failed,is udevd running?\n");
		ret = 1;
		goto out;
	}

	if (!msvdx_priv->is_load) {/* Load firmware into BO */
		PSB_DEBUG_GENERAL("MSVDX:load msvdx_fw.bin by udevd into BO\n");
		if(IS_CDV(dev)) {
			if(IS_FW_UPDATED)
				ret = msvdx_get_fw_bo(dev, &raw, "msvdx_fw_mfld_DE2.0.bin");
			else
				ret = msvdx_get_fw_bo(dev, &raw, "msvdx_fw_mfld.bin");
		}
		else
			DRM_ERROR("MSVDX:HW is neither mrst nor mfld\n");
		msvdx_priv->is_load = 1;
	}


	fw = (struct msvdx_fw *) fw_ptr;

	if(ec_firmware) {
		fw_ptr += (((sizeof(struct msvdx_fw) + (fw->text_size + fw->data_size)*4 + 0xfff) & ~0xfff)/sizeof(uint32_t));
		fw = (struct msvdx_fw *) fw_ptr;
	}

	/*
	  if (fw->ver != 0x02) {
	  DRM_ERROR("psb: msvdx_fw.bin firmware version mismatch,"
	  "got version=%02x expected version=%02x\n",
	  fw->ver, 0x02);
	  ret = 1;
	  goto out;
	  }
	*/
	text_ptr =
		(uint32_t *) ((uint8_t *) fw_ptr + sizeof(struct msvdx_fw));
	data_ptr = text_ptr + fw->text_size;

	if (fw->text_size == 2858)
		PSB_DEBUG_GENERAL(
			"MSVDX: FW ver 1.00.10.0187 of SliceSwitch variant\n");
	else if (fw->text_size == 3021)
		PSB_DEBUG_GENERAL(
			"MSVDX: FW ver 1.00.10.0187 of FrameSwitch variant\n");
	else if (fw->text_size == 2841)
		PSB_DEBUG_GENERAL("MSVDX: FW ver 1.00.10.0788\n");
	else if (fw->text_size == 3147)
		PSB_DEBUG_GENERAL("MSVDX: FW ver BUILD_DXVA_FW1.00.10.1042 of SliceSwitch variant\n");
	else if (fw->text_size == 3097)
		PSB_DEBUG_GENERAL("MSVDX: FW ver BUILD_DXVA_FW1.00.10.0963.02.0011 of FrameSwitch variant\n");
	else
		PSB_DEBUG_GENERAL("MSVDX: FW ver unknown\n");


	PSB_DEBUG_GENERAL("MSVDX: Retrieved pointers for firmware\n");
	PSB_DEBUG_GENERAL("MSVDX: text_size: %d\n", fw->text_size);
	PSB_DEBUG_GENERAL("MSVDX: data_size: %d\n", fw->data_size);
	PSB_DEBUG_GENERAL("MSVDX: data_location: 0x%x\n",
			  fw->data_location);
	PSB_DEBUG_GENERAL("MSVDX: First 4 bytes of text: 0x%x\n",
			  *text_ptr);
	PSB_DEBUG_GENERAL("MSVDX: First 4 bytes of data: 0x%x\n",
			  *data_ptr);

	PSB_DEBUG_GENERAL("MSVDX: Uploading firmware\n");
#if UPLOAD_FW_BY_DMA
	psb_upload_fw(dev_priv, 0, msvdx_priv->mtx_mem_size/4, ec_firmware);
#else
	psb_upload_fw(dev_priv, MTX_CORE_CODE_MEM, ram_bank_size,
		      PC_START_ADDRESS - MTX_CODE_BASE, fw->text_size,
		      text_ptr);
	psb_upload_fw(dev_priv, MTX_CORE_DATA_MEM, ram_bank_size,
		      fw->data_location - MTX_DATA_BASE, fw->data_size,
		      data_ptr);
#endif
#if 0
	/* todo :  Verify code upload possibly only in debug */
	ret = psb_verify_fw(dev_priv, ram_bank_size,
			    MTX_CORE_CODE_MEM,
			    PC_START_ADDRESS - MTX_CODE_BASE,
			    fw->text_size, text_ptr);
	if (ret) {
		/* Firmware code upload failed */
		ret = 1;
		goto out;
	}

	ret = psb_verify_fw(dev_priv, ram_bank_size, MTX_CORE_DATA_MEM,
			    fw->data_location - MTX_DATA_BASE,
			    fw->data_size, data_ptr);
	if (ret) {
		/* Firmware data upload failed */
		ret = 1;
		goto out;
	}
#else
	(void)psb_verify_fw;
#endif
	/*	-- Set starting PC address	*/
	psb_write_mtx_core_reg(dev_priv, MTX_PC, PC_START_ADDRESS);

	/*	-- Turn on the thread	*/
	PSB_WMSVDX32(MSVDX_MTX_ENABLE_MTX_ENABLE_MASK, MSVDX_MTX_ENABLE);

	/* Wait for the signature value to be written back */
	ret = psb_wait_for_register(dev_priv, MSVDX_COMMS_SIGNATURE,
				    MSVDX_COMMS_SIGNATURE_VALUE, /*Required value*/
				    0xffffffff, /* Enabled bits */
					1000, 100);
	if (ret) {
		DRM_ERROR("MSVDX: firmware fails to initialize.\n");
		goto out;
	}

	PSB_DEBUG_GENERAL("MSVDX: MTX Initial indications OK\n");
	PSB_DEBUG_GENERAL("MSVDX: MSVDX_COMMS_AREA_ADDR = %08x\n",
			  MSVDX_COMMS_AREA_ADDR);
#if 0

	/* Send test message */
	{
		uint32_t msg_buf[FW_VA_DEBUG_TEST2_SIZE >> 2];

		MEMIO_WRITE_FIELD(msg_buf, FW_VA_DEBUG_TEST2_MSG_SIZE,
				  FW_VA_DEBUG_TEST2_SIZE);
		MEMIO_WRITE_FIELD(msg_buf, FW_VA_DEBUG_TEST2_ID,
				  VA_MSGID_TEST2);

		ret = psb_mtx_send(dev_priv, msg_buf);
		if (ret) {
			DRM_ERROR("psb: MSVDX sending fails.\n");
			goto out;
		}

		/* Wait for Mtx to ack this message */
		psb_poll_mtx_irq(dev_priv);

	}
#endif
out:

	return ret;
}


static void psb_free_ccb(struct ttm_buffer_object **ccb)
{
	ttm_bo_unref(ccb);
	*ccb = NULL;
}

/**
 * Reset chip and disable interrupts.
 * Return 0 success, 1 failure
 */
int psb_msvdx_reset(struct drm_psb_private *dev_priv)
{
	int ret = 0;

	if(IS_CDV(dev_priv->dev)) {
		int loop;
		/* Enable Clocks */
		PSB_DEBUG_GENERAL("Enabling clocks\n");
		PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);

		/* Always pause the MMU as the core may be still active when resetting.  It is very bad to have memory 
		   activity at the same time as a reset - Very Very bad */
		PSB_WMSVDX32(2, MSVDX_MMU_CONTROL0);

		for(loop = 0; loop < 50; loop++)
			ret = psb_wait_for_register(dev_priv, MSVDX_MMU_MEM_REQ, 0,
						    0xff, 100, 10);
		if(ret)
			return ret;
	}
	{
		uint32_t ui32RegValue = 0;
		ui32RegValue = PSB_RMSVDX32(MSVDX_RENDEC_CONTROL1);
		REGIO_WRITE_FIELD(ui32RegValue, MSVDX_RENDEC_CONTROL1, RENDEC_DEC_DISABLE, 1);
		PSB_WMSVDX32(ui32RegValue, MSVDX_RENDEC_CONTROL1);
	}
	PSB_WMSVDX32((uint32_t)(~MSVDX_CONTROL_CR_MSVDX_SOFT_RESET_MASK), MSVDX_CONTROL);
	PSB_RMSVDX32(MSVDX_CONTROL);
	PSB_WMSVDX32(0, MSVDX_CONTROL);
	psb_wait_for_register(dev_priv, MSVDX_MMU_MEM_REQ, 0, 0xff, 100, 100);
		
		
	/* Issue software reset */
	/* PSB_WMSVDX32(msvdx_sw_reset_all, MSVDX_CONTROL); */
        PSB_WMSVDX32(MSVDX_CONTROL_CR_MSVDX_SOFT_RESET_MASK, MSVDX_CONTROL);

	ret = psb_wait_for_register(dev_priv, MSVDX_CONTROL, 0,
				    MSVDX_CONTROL_CR_MSVDX_SOFT_RESET_MASK, 1000, 100);

	if (!ret) {
		/* Clear interrupt enabled flag */
		PSB_WMSVDX32(0, MSVDX_HOST_INTERRUPT_ENABLE);

		/* Clear any pending interrupt flags */
		PSB_WMSVDX32(0xFFFFFFFF, MSVDX_INTERRUPT_CLEAR);
	}

	/* mutex_destroy(&msvdx_priv->msvdx_mutex); */

	return ret;
}

static int psb_allocate_ccb(struct drm_device *dev,
			    struct ttm_buffer_object **ccb,
			    uint32_t *base_addr, unsigned long size)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_bo_device *bdev = &dev_priv->bdev;
	int ret;
	struct ttm_bo_kmap_obj tmp_kmap;
	bool is_iomem;

	PSB_DEBUG_INIT("MSVDX: allocate CCB\n");

	ret = ttm_buffer_object_create(bdev, size,
				       ttm_bo_type_kernel,
				       DRM_PSB_FLAG_MEM_MMU |
				       TTM_PL_FLAG_NO_EVICT, 0, 0, 0,
				       NULL, ccb);
	if (ret) {
		DRM_ERROR("MSVDX:failed to allocate CCB.\n");
		*ccb = NULL;
		return 1;
	}

	ret = ttm_bo_kmap(*ccb, 0, (*ccb)->num_pages, &tmp_kmap);
	if (ret) {
		PSB_DEBUG_GENERAL("ttm_bo_kmap failed ret: %d\n", ret);
		ttm_bo_unref(ccb);
		*ccb = NULL;
		return 1;
	}
/*
  memset(ttm_kmap_obj_virtual(&tmp_kmap, &is_iomem), 0,
  RENDEC_A_SIZE);
*/
	memset(ttm_kmap_obj_virtual(&tmp_kmap, &is_iomem), 0,
	       size);
	ttm_bo_kunmap(&tmp_kmap);

	*base_addr = (*ccb)->offset;
	return 0;
}

static ssize_t psb_msvdx_pmstate_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);
	struct drm_psb_private *dev_priv;
	struct msvdx_private *msvdx_priv;
	unsigned int pmstate;
	unsigned long flags;
	int ret = -EINVAL;

	if (drm_dev == NULL)
		return 0;

	dev_priv = drm_dev->dev_private;
	msvdx_priv = dev_priv->msvdx_private;
	pmstate = msvdx_priv->pmstate;

	spin_lock_irqsave(&msvdx_priv->msvdx_lock, flags);
	ret = snprintf(buf, 64, "%s\n",
		       (pmstate == PSB_PMSTATE_POWERUP) ? "powerup"
		       : ((pmstate == PSB_PMSTATE_POWERDOWN) ? "powerdown"
			  : "clockgated"));
	spin_unlock_irqrestore(&msvdx_priv->msvdx_lock, flags);

	return ret;
}

static DEVICE_ATTR(msvdx_pmstate, 0444, psb_msvdx_pmstate_show, NULL);

int psb_msvdx_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	/* uint32_t clk_gate_ctrl = clk_enable_all; */
	uint32_t cmd;
	int ret;
	struct msvdx_private *msvdx_priv;
/*
	uint32_t reg_value;
	uint32_t reg_val;
*/
	if (!dev_priv->msvdx_private) {
		msvdx_priv = kmalloc(sizeof(struct msvdx_private), GFP_KERNEL);
		if (msvdx_priv == NULL)
			goto err_exit;

		dev_priv->msvdx_private = msvdx_priv;
		memset(msvdx_priv, 0, sizeof(struct msvdx_private));

		/* get device --> drm_device --> drm_psb_private --> msvdx_priv
		 * for psb_msvdx_pmstate_show: msvdx_pmpolicy
		 * if not pci_set_drvdata, can't get drm_device from device
		 */
		/* pci_set_drvdata(dev->pdev, dev); */
		if (device_create_file(&dev->pdev->dev,
				       &dev_attr_msvdx_pmstate))
			DRM_ERROR("MSVDX: could not create sysfs file\n");
		msvdx_priv->sysfs_pmstate = sysfs_get_dirent(
			dev->pdev->dev.kobj.sd, 
			NULL,
			"msvdx_pmstate");
	}

	msvdx_priv = dev_priv->msvdx_private;
	if (!msvdx_priv->ccb0) { /* one for the first time */
		/* Initialize comand msvdx queueing */
		INIT_LIST_HEAD(&msvdx_priv->msvdx_queue);
		INIT_LIST_HEAD(&msvdx_priv->deblock_queue);
		mutex_init(&msvdx_priv->msvdx_mutex);
		spin_lock_init(&msvdx_priv->msvdx_lock);
		/*figure out the stepping */
		pci_read_config_byte(dev->pdev, PSB_REVID_OFFSET, &psb_rev_id);
	}

	msvdx_priv->vec_local_mem_size = VEC_LOCAL_MEM_BYTE_SIZE;
	if (!msvdx_priv->vec_local_mem_data) {
		msvdx_priv->vec_local_mem_data = 
			kzalloc(msvdx_priv->vec_local_mem_size, GFP_KERNEL);
		if (msvdx_priv->vec_local_mem_data == NULL) {
			PSB_DEBUG_GENERAL("Vec local memory fail\n");
			goto err_exit;
		}
	}

	msvdx_priv->msvdx_busy = 0;
	msvdx_priv->msvdx_hw_busy = 1;

	/* Enable Clocks */
	PSB_DEBUG_GENERAL("Enabling clocks\n");
	PSB_WMSVDX32(clk_enable_all, MSVDX_MAN_CLK_ENABLE);

	/* Issue software reset for all but core*/
/*
	PSB_WMSVDX32((uint32_t) ~MSVDX_CORE_CR_MSVDX_CONTROL_CR_MSVDX_SOFT_RESET_MASK, REGISTER(MSVDX_CORE, CR_MSVDX_CONTROL));
	reg_value = PSB_RMSVDX32(REGISTER(MSVDX_CORE, CR_MSVDX_CONTROL));
	PSB_WMSVDX32(0, REGISTER(MSVDX_CORE, CR_MSVDX_CONTROL));
	PSB_WMSVDX32(MSVDX_CORE_CR_MSVDX_CONTROL_CR_MSVDX_SOFT_RESET_MASK, REGISTER(MSVDX_CORE, CR_MSVDX_CONTROL));

	reg_val = 0;
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_FE_MSVDX_WDT_CONTROL, FE_WDT_CNT_CTRL, 0x3);
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_FE_MSVDX_WDT_CONTROL, FE_WDT_ENABLE, 0);
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_FE_MSVDX_WDT_CONTROL, FE_WDT_ACTION0, 1);
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_FE_MSVDX_WDT_CONTROL, FE_WDT_CLEAR_SELECT, 1);
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_FE_MSVDX_WDT_CONTROL, FE_WDT_CLKDIV_SELECT, 7);
	PSB_WMSVDX32(820, REGISTER( MSVDX_CORE, CR_FE_MSVDX_WDT_COMPAREMATCH ));
	PSB_WMSVDX32(reg_val, REGISTER( MSVDX_CORE, CR_FE_MSVDX_WDT_CONTROL ));

	reg_val = 0;
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_BE_MSVDX_WDT_CONTROL, BE_WDT_CNT_CTRL, 0x7);
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_BE_MSVDX_WDT_CONTROL, BE_WDT_ENABLE, 0);
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_BE_MSVDX_WDT_CONTROL, BE_WDT_ACTION0, 1);
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_BE_MSVDX_WDT_CONTROL, BE_WDT_CLEAR_SELECT, 0xd);
	REGIO_WRITE_FIELD(reg_val, MSVDX_CORE_CR_BE_MSVDX_WDT_CONTROL, BE_WDT_CLKDIV_SELECT, 7);
	PSB_WMSVDX32(8200, REGISTER(MSVDX_CORE, CR_BE_MSVDX_WDT_COMPAREMATCH));
	PSB_WMSVDX32(reg_val, REGISTER(MSVDX_CORE, CR_BE_MSVDX_WDT_CONTROL)); 
*/
	/* Enable MMU by removing all bypass bits */
	PSB_WMSVDX32(0, MSVDX_MMU_CONTROL0);

	/* move firmware loading to the place receiving first command buffer */

	PSB_DEBUG_GENERAL("MSVDX: Setting up RENDEC,allocate CCB 0/1\n");
	/* Allocate device virtual memory as required by rendec.... */
	if (!msvdx_priv->ccb0) {
		ret = psb_allocate_ccb(dev, &msvdx_priv->ccb0,
				       &msvdx_priv->base_addr0,
				       RENDEC_A_SIZE);
		if (ret) {
			PSB_DEBUG_GENERAL("Allocate Rendec A fail\n");
			goto err_exit;
		}
	}

	if (!msvdx_priv->ccb1) {
		ret = psb_allocate_ccb(dev, &msvdx_priv->ccb1,
				       &msvdx_priv->base_addr1,
				       RENDEC_B_SIZE);
		if (ret)
			goto err_exit;
	}

	if(!msvdx_priv->fw) {
		uint32_t core_rev;
		uint32_t fw_bo_size;

		core_rev = PSB_RMSVDX32(MSVDX_CORE_REV);

		if( (core_rev&0xffffff ) < 0x020000 )
			msvdx_priv->mtx_mem_size = 16*1024;
		else
			msvdx_priv->mtx_mem_size = 40*1024;

		if(IS_CDV(dev))
			fw_bo_size =  msvdx_priv->mtx_mem_size + 4096;
		else
			fw_bo_size =  ((msvdx_priv->mtx_mem_size + 8192) & ~0xfff)*2; /* fw + ec_fw */

		PSB_DEBUG_INIT("MSVDX: MTX mem size is 0x%08xbytes  allocate firmware BO size 0x%08x\n", msvdx_priv->mtx_mem_size, 
			       fw_bo_size);

                ret = ttm_buffer_object_create(&dev_priv->bdev, fw_bo_size, /* DMA may run over a page */
                                               ttm_bo_type_kernel,
                                               DRM_PSB_FLAG_MEM_MMU | TTM_PL_FLAG_NO_EVICT,
                                               0, 0, 0, NULL, &msvdx_priv->fw);
 
		if (ret) {
			PSB_DEBUG_GENERAL("Allocate firmware BO fail\n");
			goto err_exit;
		}
	}

	PSB_DEBUG_GENERAL("MSVDX: RENDEC A: %08x RENDEC B: %08x\n",
			  msvdx_priv->base_addr0, msvdx_priv->base_addr1);

	PSB_WMSVDX32(msvdx_priv->base_addr0, MSVDX_RENDEC_BASE_ADDR0);
	PSB_WMSVDX32(msvdx_priv->base_addr1, MSVDX_RENDEC_BASE_ADDR1);

	cmd = 0;
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_BUFFER_SIZE,
			  RENDEC_BUFFER_SIZE0, RENDEC_A_SIZE / 4096);
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_BUFFER_SIZE,
			  RENDEC_BUFFER_SIZE1, RENDEC_B_SIZE / 4096);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_BUFFER_SIZE);

	if(!msvdx_priv->fw) {
		uint32_t core_rev;

		core_rev = PSB_RMSVDX32(MSVDX_CORE_REV);

		if( (core_rev&0xffffff ) < 0x020000 )
			msvdx_priv->mtx_mem_size = 16*1024;
		else
			msvdx_priv->mtx_mem_size = 40*1024;

		PSB_DEBUG_INIT("MSVDX: MTX mem size is 0x%08xbytes  allocate firmware BO size 0x%08x\n", msvdx_priv->mtx_mem_size, 
			       msvdx_priv->mtx_mem_size + 4096);

                ret = ttm_buffer_object_create(&dev_priv->bdev, msvdx_priv->mtx_mem_size + 4096, /* DMA may run over a page */
                                               ttm_bo_type_kernel,
                                               DRM_PSB_FLAG_MEM_MMU | TTM_PL_FLAG_NO_EVICT,
                                               0, 0, 0, NULL, &msvdx_priv->fw);
 
		if (ret) {
			PSB_DEBUG_GENERAL("Allocate firmware BO fail\n");
			goto err_exit;
		}
	}

	cmd = 0;
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL1,
			  RENDEC_DECODE_START_SIZE, 0);
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL1,
			  RENDEC_BURST_SIZE_W, 1);
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL1,
			  RENDEC_BURST_SIZE_R, 1);
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL1,
			  RENDEC_EXTERNAL_MEMORY, 1);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTROL1);

	cmd = 0x00101010;
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT0);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT1);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT2);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT3);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT4);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTEXT5);

	cmd = 0;
	REGIO_WRITE_FIELD(cmd, MSVDX_RENDEC_CONTROL0, RENDEC_INITIALISE,
			  1);
	PSB_WMSVDX32(cmd, MSVDX_RENDEC_CONTROL0);

	/* PSB_WMSVDX32(clk_enable_minimal, MSVDX_MAN_CLK_ENABLE); */
	PSB_DEBUG_INIT("MSVDX:defer firmware loading to the"
		       " place when receiving user space commands\n");

	msvdx_priv->msvdx_fw_loaded = 0; /* need to load firware */

	PSB_WMSVDX32(820, MSVDX_CORE_CR_FE_MSVDX_WDT_COMPAREMATCH);
	PSB_WMSVDX32(8200, MSVDX_CORE_CR_BE_MSVDX_WDT_COMPAREMATCH);

	PSB_WMSVDX32(820, MSVDX_CORE_CR_FE_MSVDX_WDT_COMPAREMATCH);
	PSB_WMSVDX32(8200, MSVDX_CORE_CR_BE_MSVDX_WDT_COMPAREMATCH);

	psb_msvdx_clearirq(dev);
	psb_msvdx_enableirq(dev);

	if (IS_MSVDX(dev)) {
		PSB_DEBUG_INIT("MSDVX:old clock gating disable = 0x%08x\n",
			       PSB_RVDC32(PSB_MSVDX_CLOCKGATING));
	}

	{
		cmd = 0;
		cmd = PSB_RMSVDX32(MSVDX_VEC_SHIFTREG_CONTROL); /* VEC_SHIFTREG_CONTROL */
		REGIO_WRITE_FIELD(cmd,
				  VEC_SHIFTREG_CONTROL,
				  SR_MASTER_SELECT,
				  1);  /* Host */
		PSB_WMSVDX32(cmd, MSVDX_VEC_SHIFTREG_CONTROL);
	}

#if 0
	ret = psb_setup_fw(dev);
	if (ret)
		goto err_exit;
	/* Send Initialisation message to firmware */
	if (0) {
		uint32_t msg_init[FW_VA_INIT_SIZE >> 2];
		MEMIO_WRITE_FIELD(msg_init, FWRK_GENMSG_SIZE,
				  FW_VA_INIT_SIZE);
		MEMIO_WRITE_FIELD(msg_init, FWRK_GENMSG_ID, VA_MSGID_INIT);

		/* Need to set this for all but A0 */
		MEMIO_WRITE_FIELD(msg_init, FW_VA_INIT_GLOBAL_PTD,
				  psb_get_default_pd_addr(dev_priv->mmu));

		ret = psb_mtx_send(dev_priv, msg_init);
		if (ret)
			goto err_exit;

		psb_poll_mtx_irq(dev_priv);
	}
#endif

	return 0;

err_exit:
	DRM_ERROR("MSVDX: initialization failed\n");
	if (msvdx_priv && msvdx_priv->ccb0)
		psb_free_ccb(&msvdx_priv->ccb0);
	if (msvdx_priv && msvdx_priv->ccb1)
		psb_free_ccb(&msvdx_priv->ccb1);
	if (dev_priv->msvdx_private) {
		kfree(dev_priv->msvdx_private);
		dev_priv->msvdx_private = NULL;
	}
	return 1;
}

int psb_msvdx_uninit(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;

	/* Reset MSVDX chip */
	psb_msvdx_reset(dev_priv);

	/* PSB_WMSVDX32 (clk_enable_minimal, MSVDX_MAN_CLK_ENABLE); */
	PSB_DEBUG_INIT("MSVDX:set the msvdx clock to 0\n");
	PSB_WMSVDX32(0, MSVDX_MAN_CLK_ENABLE);

	if (NULL == msvdx_priv)
	{
		DRM_ERROR("MSVDX: psb_msvdx_uninit: msvdx_priv is NULL!\n");
		return -1;
	}

	if (msvdx_priv->ccb0)
		psb_free_ccb(&msvdx_priv->ccb0);
	if (msvdx_priv->ccb1)
		psb_free_ccb(&msvdx_priv->ccb1);
	if (msvdx_priv->msvdx_fw)
		kfree(msvdx_priv->msvdx_fw
			);
	if (msvdx_priv->vec_local_mem_data)
		kfree(msvdx_priv->vec_local_mem_data);

	if (msvdx_priv) {
		/* pci_set_drvdata(dev->pdev, NULL); */
		device_remove_file(&dev->pdev->dev, &dev_attr_msvdx_pmstate);
		sysfs_put(msvdx_priv->sysfs_pmstate);
		msvdx_priv->sysfs_pmstate = NULL;

		kfree(msvdx_priv);
		dev_priv->msvdx_private = NULL;
	}

	return 0;
}
