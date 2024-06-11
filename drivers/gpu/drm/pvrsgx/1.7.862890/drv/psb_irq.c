/**************************************************************************
 * Copyright (c) 2011, Intel Corporation.
 * All Rights Reserved.
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
 * Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 * develop this driver.
 *
 **************************************************************************/
/*
 */

#include <drm/drmP.h>
#include "psb_drv.h"
#include "psb_reg.h"
#include "psb_msvdx.h"
#include "psb_intel_reg.h"
#include "psb_powermgmt.h"


/*
 * inline functions
 */
static inline u32
psb_pipestat(int pipe)
{
	if (pipe == 0)
		return PIPEASTAT;
	if (pipe == 1)
		return PIPEBSTAT;
	BUG();
}

static inline u32
psb_pipe_event(int pipe)
{
	if (pipe == 0)
		return PSB_IRQ_PIPEA_EVENT;
	if (pipe == 1)
		return PSB_IRQ_PIPEB_EVENT;
	BUG();
}

static inline u32
psb_pipeconf(int pipe)
{
	if (pipe == 0)
		return PIPEACONF;
	if (pipe == 1)
		return PIPEBCONF;
	BUG();
}

void
psb_enable_pipestat(struct drm_psb_private *dev_priv, int pipe, u32 mask)
{
	if ((dev_priv->pipestat[pipe] & mask) != mask) {
		u32 reg = psb_pipestat(pipe);
		u32 writeVal = PSB_RVDC32(reg);

		dev_priv->pipestat[pipe] |= mask;
		/* Enable the interrupt, clear any pending status */
		writeVal |= (mask | (mask >> 16));
		PSB_WVDC32(writeVal, reg);
		(void) PSB_RVDC32(reg);
	}
}

void
psb_disable_pipestat(struct drm_psb_private *dev_priv, int pipe, u32 mask)
{
	if ((dev_priv->pipestat[pipe] & mask) != 0) {
		u32 reg = psb_pipestat(pipe);
		u32 writeVal = PSB_RVDC32(reg);

		dev_priv->pipestat[pipe] &= ~mask;
		writeVal &= ~mask;
		PSB_WVDC32(writeVal, reg);
		(void) PSB_RVDC32(reg);
	}
}

void
mid_enable_pipe_event(struct drm_psb_private *dev_priv, int pipe)
{
	u32 pipe_event = psb_pipe_event(pipe);
	dev_priv->vdc_irq_mask |= pipe_event;
	PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);
	PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
}

void
mid_disable_pipe_event(struct drm_psb_private *dev_priv, int pipe)
{
	if (dev_priv->pipestat[pipe] == 0) {
		u32 pipe_event = psb_pipe_event(pipe);
		dev_priv->vdc_irq_mask &= ~pipe_event;
		PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);
		PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);
	}
}

/**
 * Display controller interrupt handler for vsync/vblank.
 *
 */
static void mid_vblank_handler(struct drm_device *dev, uint32_t pipe)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	/**
	 * Using TE interrupt for B0 + command mode panels
	 */

	drm_handle_vblank(dev, pipe);
	
	if( dev_priv->psb_vsync_handler != NULL)
		(*dev_priv->psb_vsync_handler)(dev,pipe);
}

/**
 * Display controller interrupt handler for pipe event.
 *
 */
#define WAIT_STATUS_CLEAR_LOOP_COUNT 0xffff
static void mid_pipe_event_handler(struct drm_device *dev, uint32_t pipe)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	uint32_t pipe_stat_val = 0;
	uint32_t pipe_stat_reg = psb_pipestat(pipe);
	uint32_t pipe_enable = dev_priv->pipestat[pipe];
	uint32_t pipe_status = dev_priv->pipestat[pipe] >> 16;
	uint32_t i = 0, temp;
	unsigned long irq_flags;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irq_flags);

	pipe_stat_val = PSB_RVDC32(pipe_stat_reg);
	pipe_stat_val &= pipe_enable | pipe_status;
	pipe_stat_val &= pipe_stat_val >> 16;


	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irq_flags);
	temp = PSB_RVDC32(pipe_stat_reg);
        temp &=~(0xFFFF);
        if (temp & PIPE_FIFO_UNDERRUN) {
		DRM_DEBUG_KMS("Fifo underrun on pipe %d\n", pipe);
                PSB_WVDC32(temp | PIPE_FIFO_UNDERRUN, pipe_stat_reg);
		PSB_RVDC32(pipe_stat_reg);
	}
	/* clear the 2nd level interrupt status bits */
	/**
	* FIXME: shouldn't use while loop here. However, the interrupt
	* status 'sticky' bits cannot be cleared by setting '1' to that
	* bit once...
	*/
	for (i = 0; i < WAIT_STATUS_CLEAR_LOOP_COUNT; i ++) {
		PSB_WVDC32(PSB_RVDC32(pipe_stat_reg), pipe_stat_reg);
		(void) PSB_RVDC32(pipe_stat_reg);

		if ((PSB_RVDC32(pipe_stat_reg) & pipe_status) == 0)
			break;
	}

	if (i == WAIT_STATUS_CLEAR_LOOP_COUNT)
		DRM_ERROR("%s, can't clear the status bits in pipe_stat_reg, its value = 0x%x. \n",
			__FUNCTION__, PSB_RVDC32(pipe_stat_reg));

	if ((pipe_stat_val & PIPE_DPST_EVENT_STATUS) &&
	   (dev_priv->psb_dpst_state != NULL)) {
		uint32_t pwm_reg = 0;
		uint32_t hist_reg = 0;
		u32 irqCtrl = 0;
		struct dpst_guardband guardband_reg;
		struct dpst_ie_histogram_control ie_hist_cont_reg;

		hist_reg = PSB_RVDC32(HISTOGRAM_INT_CONTROL);

		/* Determine if this is histogram or pwm interrupt */
		if(hist_reg & HISTOGRAM_INT_CTRL_CLEAR) {
			/* Notify UM of histogram interrupt */
			psb_dpst_notify_change_um(DPST_EVENT_HIST_INTERRUPT,
			dev_priv->psb_dpst_state);

			/* disable dpst interrupts */
			guardband_reg.data = PSB_RVDC32(HISTOGRAM_INT_CONTROL);
			guardband_reg.interrupt_enable = 0;
			guardband_reg.interrupt_status = 1;
			PSB_WVDC32(guardband_reg.data, HISTOGRAM_INT_CONTROL);

			ie_hist_cont_reg.data = PSB_RVDC32(HISTOGRAM_LOGIC_CONTROL);
			ie_hist_cont_reg.ie_histogram_enable = 0;
			PSB_WVDC32(ie_hist_cont_reg.data, HISTOGRAM_LOGIC_CONTROL);

			irqCtrl = PSB_RVDC32(PIPEASTAT);
			irqCtrl &= ~PIPE_DPST_EVENT_ENABLE;
			PSB_WVDC32(irqCtrl, PIPEASTAT);
		}
		pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);
		if((pwm_reg & PWM_PHASEIN_INT_ENABLE) &&
		   !(pwm_reg & PWM_PHASEIN_ENABLE)) {
			/* Notify UM of the phase complete */
			psb_dpst_notify_change_um(DPST_EVENT_PHASE_COMPLETE,
			dev_priv->psb_dpst_state);

			/* Temporarily get phase mngr ready to generate
			 * another interrupt until this can be moved to
			 * user mode */
			/* PSB_WVDC32(pwm_reg | 0x80010100 | PWM_PHASEIN_ENABLE,
				   PWM_CONTROL_LOGIC); */
		}
	}

	if (pipe_stat_val & PIPE_VBLANK_STATUS) {
		mid_vblank_handler(dev, pipe);
	}
}

/**
 * Display controller interrupt handler.
 */
static void psb_vdc_interrupt(struct drm_device *dev, uint32_t vdc_stat)
{
	if (vdc_stat & PSB_IRQ_ASLE)
		psb_intel_opregion_asle_intr(dev);

	if (vdc_stat & PSB_IRQ_PIPEA_EVENT)
		mid_pipe_event_handler(dev, 0);

	if (vdc_stat & PSB_IRQ_PIPEB_EVENT)
		mid_pipe_event_handler(dev, 1);
}

static void psb_hotplug_work_func(struct work_struct *work)
{
        struct drm_psb_private *dev_priv = container_of(work, struct drm_psb_private,
							hotplug_work);                 
        struct drm_device *dev = dev_priv->dev;

        /* Just fire off a uevent and let userspace tell us what to do */
        drm_helper_hpd_irq_event(dev);
}                       

irqreturn_t psb_irq_handler(DRM_IRQ_ARGS)
{
	struct drm_device *dev = (struct drm_device *) arg;
	struct drm_psb_private *dev_priv = (struct drm_psb_private *) dev->dev_private;
	uint32_t vdc_stat, dsp_int = 0, sgx_int = 0, msvdx_int = 0, hotplug_int = 0;
	int handled = 0;
	unsigned long irq_flags;

/*	PSB_DEBUG_ENTRY("\n"); */

	spin_lock_irqsave(&dev_priv->irqmask_lock, irq_flags);

	vdc_stat = PSB_RVDC32(PSB_INT_IDENTITY_R);

	if (vdc_stat & _PSB_DISP_ALL_IRQ_FLAG) {
		PSB_DEBUG_IRQ("Got DISP interrupt\n");
		dsp_int = 1;
	}

	if (vdc_stat & _PSB_IRQ_SGX_FLAG) {
		PSB_DEBUG_IRQ("Got SGX interrupt\n");
		sgx_int = 1;
	}

	if (vdc_stat & _PSB_IRQ_MSVDX_FLAG) {
		PSB_DEBUG_IRQ("Got MSVDX interrupt\n");
		msvdx_int = 1;
	}

	if (vdc_stat & PSB_IRQ_DISP_HOTSYNC) {
		PSB_DEBUG_IRQ("Got hotplug interrupt\n");
		hotplug_int = 1;
	}

	vdc_stat &= dev_priv->vdc_irq_mask;
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irq_flags);

	if (dsp_int) {
		psb_vdc_interrupt(dev, vdc_stat);
		handled = 1;
	}

	if (msvdx_int && ospm_power_is_hw_on(OSPM_VIDEO_DEC_ISLAND)) {
		psb_msvdx_interrupt(dev);
		handled = 1;
	}

	if (sgx_int) {
		if (SYSPVRServiceSGXInterrupt(dev) != 0)
			handled = 1;
	}

	if (hotplug_int) {
		/* use system wq for now*/
		schedule_work(&dev_priv->hotplug_work);
		REG_WRITE(PORT_HOTPLUG_STAT, REG_READ(PORT_HOTPLUG_STAT));
		handled = 1;
	}

	PSB_WVDC32(vdc_stat, PSB_INT_IDENTITY_R);
	(void) PSB_RVDC32(PSB_INT_IDENTITY_R);
	DRM_READMEMORYBARRIER();

	if (!handled)
		return IRQ_NONE;

	return IRQ_HANDLED;
}

void psb_irq_preinstall(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = (struct drm_psb_private *) dev->dev_private;
	psb_irq_preinstall_islands(dev, OSPM_ALL_ISLANDS);

	INIT_WORK(&dev_priv->hotplug_work, psb_hotplug_work_func);
	
	REG_WRITE(PORT_HOTPLUG_EN, 0);
	REG_WRITE(PORT_HOTPLUG_STAT, REG_READ(PORT_HOTPLUG_STAT));
}

/**
 * FIXME: should I remove display irq enable here??
 */
void psb_irq_preinstall_islands(struct drm_device *dev, int hw_islands)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;

	/* PSB_DEBUG_ENTRY("\n"); */

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	if (dev->vblank_enabled[0])
		dev_priv->vdc_irq_mask |= PSB_IRQ_PIPEA_EVENT;
	if (dev->vblank_enabled[1])
		dev_priv->vdc_irq_mask |= PSB_IRQ_PIPEB_EVENT;

	if (hw_islands & OSPM_GRAPHICS_ISLAND) {
		dev_priv->vdc_irq_mask |= _PSB_IRQ_SGX_FLAG;
	}

	if (hw_islands & OSPM_VIDEO_DEC_ISLAND)
		if (ospm_power_is_hw_on(OSPM_VIDEO_DEC_ISLAND))
			dev_priv->vdc_irq_mask |= _PSB_IRQ_MSVDX_FLAG;

	/* display hotplug */
	dev_priv->vdc_irq_mask |= PSB_IRQ_DISP_HOTSYNC;

	/* asle interrupt */
	dev_priv->vdc_irq_mask |= PSB_IRQ_ASLE;

	/*This register is safe even if display island is off*/
	/* unmask all */
	PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

int psb_irq_postinstall(struct drm_device *dev)
{
	return psb_irq_postinstall_islands(dev, OSPM_ALL_ISLANDS);
}

int psb_irq_postinstall_islands(struct drm_device *dev, int hw_islands)
{

	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;
	u32 temp;

	/* PSB_DEBUG_ENTRY("\n"); */

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	/*This register is safe even if display island is off*/
	PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);

	if (true/*powermgmt_is_hw_on(dev->pdev, PSB_DISPLAY_ISLAND)*/) {	
		if (dev->vblank_enabled[0])
			psb_enable_pipestat(dev_priv, 0,
					    PIPE_VBLANK_INTERRUPT_ENABLE);
		else
			psb_disable_pipestat(dev_priv, 0,
					     PIPE_VBLANK_INTERRUPT_ENABLE);

		if (dev->vblank_enabled[1])
			psb_enable_pipestat(dev_priv, 1,
					    PIPE_VBLANK_INTERRUPT_ENABLE);
		else
			psb_disable_pipestat(dev_priv, 1,
					     PIPE_VBLANK_INTERRUPT_ENABLE);
	}

	/* Just enable all display ports for now.
	 * This can be optimized to only enable ports that really exists.
	 */
	temp = REG_READ(PORT_HOTPLUG_EN);
	temp |= HDMIB_HOTPLUG_INT_EN; 
	temp |= HDMIC_HOTPLUG_INT_EN; 
	temp |= HDMID_HOTPLUG_INT_EN; 
	temp |= CRT_HOTPLUG_INT_EN; 
	REG_WRITE(PORT_HOTPLUG_EN, temp);

	if (hw_islands & OSPM_VIDEO_DEC_ISLAND)
		if (true/*powermgmt_is_hw_on(dev->pdev, PSB_VIDEO_DEC_ISLAND)*/)
			psb_msvdx_enableirq(dev);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);

	return 0;
}

void psb_irq_uninstall(struct drm_device *dev)
{
	psb_irq_uninstall_islands(dev, OSPM_ALL_ISLANDS);
}

void psb_irq_uninstall_islands(struct drm_device *dev, int hw_islands)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;

	/* PSB_DEBUG_ENTRY("\n"); */

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);
	
	if (true/*powermgmt_is_hw_on(dev->pdev, PSB_DISPLAY_ISLAND)*/) {
		if (dev->vblank_enabled[0])
			psb_disable_pipestat(dev_priv, 0,
					     PIPE_VBLANK_INTERRUPT_ENABLE);

		if (dev->vblank_enabled[1])
			psb_disable_pipestat(dev_priv, 1,
					     PIPE_VBLANK_INTERRUPT_ENABLE);

	}

	psb_disable_pipestat(dev_priv, 1, PIPE_LEGACY_BLC_EVENT_ENABLE);

	REG_WRITE(PORT_HOTPLUG_EN, 0);
	REG_WRITE(PORT_HOTPLUG_STAT, REG_READ(PORT_HOTPLUG_STAT));

	dev_priv->vdc_irq_mask &= _PSB_IRQ_SGX_FLAG |
				  _PSB_IRQ_MSVDX_FLAG;

	/*TODO: remove follwoing code*/
	if (hw_islands & OSPM_GRAPHICS_ISLAND) {
		dev_priv->vdc_irq_mask &= ~_PSB_IRQ_SGX_FLAG;
	}

	if ((hw_islands & OSPM_VIDEO_DEC_ISLAND))
		dev_priv->vdc_irq_mask &= ~_PSB_IRQ_MSVDX_FLAG;

	/*These two registers are safe even if display island is off*/
	PSB_WVDC32(~dev_priv->vdc_irq_mask, PSB_INT_MASK_R);
	PSB_WVDC32(dev_priv->vdc_irq_mask, PSB_INT_ENABLE_R);

	wmb();

	/*This register is safe even if display island is off*/
	PSB_WVDC32(PSB_RVDC32(PSB_INT_IDENTITY_R), PSB_INT_IDENTITY_R);

	if (hw_islands & OSPM_VIDEO_DEC_ISLAND)
		if (ospm_power_is_hw_on(OSPM_VIDEO_DEC_ISLAND))
			psb_msvdx_disableirq(dev);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

void psb_irq_turn_on_dpst(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;
	u32 hist_reg;
	u32 pwm_reg;

	PSB_WVDC32(BIT31, HISTOGRAM_LOGIC_CONTROL);
	hist_reg = PSB_RVDC32(HISTOGRAM_LOGIC_CONTROL);
	PSB_WVDC32(BIT31, HISTOGRAM_INT_CONTROL);
	hist_reg = PSB_RVDC32(HISTOGRAM_INT_CONTROL);

	PSB_WVDC32(0x80010100, PWM_CONTROL_LOGIC);
	pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);
	PSB_WVDC32(pwm_reg | PWM_PHASEIN_ENABLE | PWM_PHASEIN_INT_ENABLE,
		   PWM_CONTROL_LOGIC);
	pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);

	/* pipe B */
	psb_enable_pipestat(dev_priv, 1, PIPE_DPST_EVENT_ENABLE);

	hist_reg = PSB_RVDC32(HISTOGRAM_INT_CONTROL);
	PSB_WVDC32(hist_reg | HISTOGRAM_INT_CTRL_CLEAR,HISTOGRAM_INT_CONTROL);
	pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);
	PSB_WVDC32(pwm_reg | 0x80010100 | PWM_PHASEIN_ENABLE, PWM_CONTROL_LOGIC);
		
}

int psb_irq_enable_dpst(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;

	PSB_DEBUG_ENTRY("\n");

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	/* enable DPST */
	/* DPST always on LVDS pipe B */
	mid_enable_pipe_event(dev_priv, 1);
	psb_irq_turn_on_dpst(dev);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
	return 0;
}

void psb_irq_turn_off_dpst(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	u32 pwm_reg;

	PSB_WVDC32(0x00000000, HISTOGRAM_INT_CONTROL);

	/* on pipe B */
	psb_disable_pipestat(dev_priv, 1, PIPE_DPST_EVENT_ENABLE);

	pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);
	PSB_WVDC32(pwm_reg & !(PWM_PHASEIN_INT_ENABLE), PWM_CONTROL_LOGIC);
	pwm_reg = PSB_RVDC32(PWM_CONTROL_LOGIC);
}

int psb_irq_disable_dpst(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;

	PSB_DEBUG_ENTRY("\n");

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	/* on pipe B */
	mid_disable_pipe_event(dev_priv, 1);
	psb_irq_turn_off_dpst(dev);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);

	return 0;
}

/*
 * It is used to enable VBLANK interrupt
 */
int psb_enable_vblank(struct drm_device *dev, int pipe)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;
	uint32_t reg_val = 0;
	uint32_t pipeconf_reg = psb_pipeconf(pipe);

	PSB_DEBUG_ENTRY("\n");


	reg_val = REG_READ(pipeconf_reg);

	if (!(reg_val & PIPEACONF_ENABLE))
		return -EINVAL;

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	mid_enable_pipe_event(dev_priv, pipe);
	psb_enable_pipestat(dev_priv, pipe, PIPE_VBLANK_INTERRUPT_ENABLE);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);

	return 0;
}

/*
 * It is used to disable VBLANK interrupt
 */
void psb_disable_vblank(struct drm_device *dev, int pipe)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	unsigned long irqflags;

	PSB_DEBUG_ENTRY("\n");

	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);

	mid_disable_pipe_event(dev_priv, pipe);
	psb_disable_pipestat(dev_priv, pipe, PIPE_VBLANK_INTERRUPT_ENABLE);

	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
}

/* Called from drm generic code, passed a 'crtc', which
 * we use as a pipe index
 */
u32 psb_get_vblank_counter(struct drm_device *dev, int pipe)
{
	uint32_t high_frame = PIPEAFRAMEHIGH;
	uint32_t low_frame = PIPEAFRAMEPIXEL;
	uint32_t pipeconf_reg = PIPEACONF;
	uint32_t reg_val = 0;
	uint32_t high1 = 0, high2 = 0, low = 0, count = 0;

	switch (pipe) {
	case 0:
		break;
	case 1:
		high_frame = PIPEBFRAMEHIGH;
		low_frame = PIPEBFRAMEPIXEL;
		pipeconf_reg = PIPEBCONF;
		break;
	case 2:
		high_frame = PIPECFRAMEHIGH;
		low_frame = PIPECFRAMEPIXEL;
		pipeconf_reg = PIPECCONF;
		break;
	default:
		DRM_ERROR("%s, invalded pipe. \n", __FUNCTION__);
		return 0;
	}

	reg_val = REG_READ(pipeconf_reg);

	if (!(reg_val & PIPEACONF_ENABLE)) {
		DRM_DEBUG_DRIVER("trying to get vblank count for disabled pipe %d\n", pipe);
		goto psb_get_vblank_counter_exit;
	}

	/*
	 * High & low register fields aren't synchronized, so make sure
	 * we get a low value that's stable across two reads of the high
	 * register.
	 */
	do {
		high1 = ((REG_READ(high_frame) & PIPE_FRAME_HIGH_MASK) >>
			 PIPE_FRAME_HIGH_SHIFT);
		low =  ((REG_READ(low_frame) & PIPE_FRAME_LOW_MASK) >>
			PIPE_FRAME_LOW_SHIFT);
		high2 = ((REG_READ(high_frame) & PIPE_FRAME_HIGH_MASK) >>
			 PIPE_FRAME_HIGH_SHIFT);
	} while (high1 != high2);

	count = (high1 << 8) | low;

psb_get_vblank_counter_exit:

	return count;
}

/* 
 * psb_intel_enable_asle - enable ASLE interrupt for OpRegion
 */
void psb_intel_enable_asle(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = (struct drm_psb_private *) dev->dev_private;

	DRM_DEBUG_DRIVER("enable ASLE\n");

	/* Only enable for pipe B which LVDS always binds to */
	psb_enable_pipestat(dev_priv, 1, PIPE_LEGACY_BLC_EVENT_ENABLE);
}

