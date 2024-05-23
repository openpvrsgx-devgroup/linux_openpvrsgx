/**************************************************************************
 * Copyright (c) 2011, Intel Corporation.
 * All Rights Reserved.

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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Benjamin Defnet <benjamin.r.defnet@intel.com>
 *    Rajesh Poornachandran <rajesh.poornachandran@intel.com>
 *
 */
#include "psb_powermgmt.h"
#include "psb_drv.h"
#include "psb_intel_reg.h"
#include "psb_msvdx.h"
#include <linux/mutex.h>
#include <asm/intel_scu_ipc.h>
#include <linux/pm_runtime.h>

extern IMG_UINT32 gui32SGXDeviceID;
extern IMG_UINT32 gui32MRSTDisplayDeviceID;
extern IMG_UINT32 gui32MRSTMSVDXDeviceID;
extern IMG_UINT32 gui32MRSTTOPAZDeviceID;

struct drm_device *gpDrmDevice = NULL;
static struct mutex g_ospm_mutex;
static bool gbSuspendInProgress = false;
static bool gbResumeInProgress = false;
static int g_hw_power_status_mask;
static atomic_t g_graphics_access_count;
static atomic_t g_videodec_access_count;
int allow_runtime_pm = 0;

void ospm_power_island_up(int hw_islands);
void ospm_power_island_down(int hw_islands);
static bool gbSuspended = false;
bool gbgfxsuspended = false;
extern int enter_dsr;

#if 1
static int ospm_runtime_check_msvdx_hw_busy(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;
	int ret = 1;
	
	if (!ospm_power_is_hw_on(OSPM_VIDEO_DEC_ISLAND)) {
		/*printk(KERN_ALERT "%s VIDEO DEC HW is not on\n", __func__); */
		ret = -1;
		goto out;
	}
	
	msvdx_priv->msvdx_hw_busy = REG_READ(0x20D0) & (0x1 << 9);
	if (psb_check_msvdx_idle(dev)){
		/*printk(KERN_ALERT "%s video decode hw busy\n", __func__); */
		ret = 1;
	}
	else {
		/*printk(KERN_ALERT "%s video decode hw idle\n", __func__); */
		ret = 0;
	}
out:
	return ret;
}

static int ospm_runtime_pm_msvdx_suspend(struct drm_device *dev)
{
	int ret = 0;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;

	/*printk(KERN_ALERT "enter %s\n", __func__);*/

	if (!ospm_power_is_hw_on(OSPM_VIDEO_DEC_ISLAND)) {
		/*printk(KERN_ALERT "%s VIDEO DEC HW is not on\n", __func__);*/
		goto out;
	}

	if (atomic_read(&g_videodec_access_count)) {
		/*printk(KERN_ALERT "%s videodec access count exit\n", __func__);*/
		ret = -1;
		goto out;
	}

	msvdx_priv->msvdx_hw_busy = REG_READ(0x20D0) & (0x1 << 9);
	if (psb_check_msvdx_idle(dev)){
		/*printk(KERN_ALERT "%s video decode hw busy exit\n", __func__);*/
		ret = -2;
		goto out;
	}
	
	MSVDX_NEW_PMSTATE(dev, msvdx_priv, PSB_PMSTATE_POWERDOWN);
	psb_irq_uninstall_islands(gpDrmDevice, OSPM_VIDEO_DEC_ISLAND);
	psb_msvdx_save_context(dev);
	ospm_power_island_down(OSPM_VIDEO_DEC_ISLAND);
	/*printk(KERN_ALERT "%s done\n", __func__);*/
out:
	return ret;
}

static int ospm_runtime_pm_msvdx_resume(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;
	
	/*printk(KERN_ALERT "ospm_runtime_pm_msvdx_resume\n");*/
	
	MSVDX_NEW_PMSTATE(dev, msvdx_priv, PSB_PMSTATE_POWERUP);
	
	psb_msvdx_restore_context(dev);

	return 0;
}

#endif

#ifdef FIX_OSPM_POWER_DOWN
void ospm_apm_power_down_msvdx(struct drm_device *dev)
{
	return;
	mutex_lock(&g_ospm_mutex);

	if (atomic_read(&g_videodec_access_count))
		goto out;
	if (psb_check_msvdx_idle(dev))
		goto out;

	gbSuspendInProgress = true;
	psb_msvdx_save_context(dev);
#ifdef FIXME_MRST_VIDEO_DEC
	ospm_power_island_down(OSPM_VIDEO_DEC_ISLAND);
#endif
	gbSuspendInProgress = false;
out:
	mutex_unlock(&g_ospm_mutex);
	return;
}

#else
void ospm_apm_power_down_msvdx(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;

	mutex_lock(&g_ospm_mutex);
	if (!ospm_power_is_hw_on(OSPM_VIDEO_DEC_ISLAND))
		goto out;
	
	if (atomic_read(&g_videodec_access_count))
		goto out;
	if (psb_check_msvdx_idle(dev))
		goto out;

	gbSuspendInProgress = true;
	psb_msvdx_save_context(dev);
	ospm_power_island_down(OSPM_VIDEO_DEC_ISLAND);
	gbSuspendInProgress = false;
	MSVDX_NEW_PMSTATE(dev, msvdx_priv, PSB_PMSTATE_POWERDOWN);
out:
	mutex_unlock(&g_ospm_mutex);
	return;
}

#endif
/*
 * ospm_power_init
 *
 * Description: Initialize this ospm power management module
 */
void ospm_power_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = (struct drm_psb_private *)dev->dev_private;

	gpDrmDevice = dev;

	if (IS_CDV(dev))
	{
		dev_priv->apm_reg = CDV_MSG_READ32(PSB_PUNIT_PORT, PSB_APMBA);
		dev_priv->ospm_base = CDV_MSG_READ32(PSB_PUNIT_PORT, PSB_OSPMBA);
	}

	DRM_DEBUG_KMS("CDV: power apm io base 0x%x, ospm io base 0x%x\n", dev_priv->apm_reg,
		  dev_priv->ospm_base);

	dev_priv->apm_base = dev_priv->apm_reg & 0xffff;
	dev_priv->ospm_base &= 0xffff;

	mutex_init(&g_ospm_mutex);
	g_hw_power_status_mask = OSPM_GRAPHICS_ISLAND | OSPM_VIDEO_DEC_ISLAND;
	atomic_set(&g_graphics_access_count, 0);
	atomic_set(&g_videodec_access_count, 0);

#ifdef OSPM_STAT
	dev_priv->graphics_state = PSB_PWR_STATE_ON;
	dev_priv->gfx_last_mode_change = jiffies;
	dev_priv->gfx_on_time = 0;
	dev_priv->gfx_off_time = 0;
#endif
}

/*
 * ospm_power_uninit
 *
 * Description: Uninitialize this ospm power management module
 */
void ospm_power_uninit(void)
{
	mutex_destroy(&g_ospm_mutex);
    	pm_runtime_disable(&gpDrmDevice->pdev->dev);
	pm_runtime_set_suspended(&gpDrmDevice->pdev->dev);
}

/*
 * helper function to turn on/off outputs.
 */
static void ospm_output_dpms(struct drm_device *dev, bool on)
{
	int state = on ? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF;
	struct drm_connector *connector;
	
	list_for_each_entry(connector, &dev->mode_config.connector_list, head)
		connector->funcs->dpms(connector, state);
}

/*
 * powermgmt_suspend_display
 *
 * Description: Suspend the display hardware saving state and disabling
 * as necessary.
 */
void ospm_suspend_display(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	DRM_DEBUG_KMS("\n");

	pci_read_config_byte(dev->pdev, 0xF4, &dev_priv->saveLBB);

	mutex_lock(&dev->struct_mutex);

	dev_priv->saveDSPCLK_GATE_D = REG_READ(DSPCLK_GATE_D);
	dev_priv->saveRAMCLK_GATE_D = REG_READ(RAMCLK_GATE_D);

	dev_priv->saveDSPARB = REG_READ(DSPARB);
	dev_priv->saveDSPFW[0] = REG_READ(DSPFW1);
	dev_priv->saveDSPFW[1] = REG_READ(DSPFW2);
	dev_priv->saveDSPFW[2] = REG_READ(DSPFW3);
	dev_priv->saveDSPFW[3] = REG_READ(DSPFW4);
	dev_priv->saveDSPFW[4] = REG_READ(DSPFW5);
	dev_priv->saveDSPFW[5] = REG_READ(DSPFW6);

	dev_priv->saveADPA = REG_READ(ADPA);

	dev_priv->savePP_CONTROL = REG_READ(PP_CONTROL);
	dev_priv->savePFIT_PGM_RATIOS = REG_READ(PFIT_PGM_RATIOS);
	dev_priv->saveBLC_PWM_CTL = REG_READ(BLC_PWM_CTL);
	dev_priv->saveBLC_PWM_CTL2 = REG_READ(BLC_PWM_CTL2);
	dev_priv->saveLVDS = REG_READ(LVDS);

	dev_priv->savePFIT_CONTROL = REG_READ(PFIT_CONTROL);

	dev_priv->savePP_ON_DELAYS = REG_READ(PP_ON_DELAYS);
	dev_priv->savePP_OFF_DELAYS = REG_READ(PP_OFF_DELAYS);
	dev_priv->savePP_DIVISOR = REG_READ(PP_DIVISOR);

	dev_priv->saveVGACNTRL = REG_READ(VGACNTRL);

	dev_priv->saveIER = REG_READ(PSB_INT_ENABLE_R);
	dev_priv->saveIMR = REG_READ(PSB_INT_MASK_R);

	ospm_output_dpms(dev, false);
 
	mutex_unlock(&dev->struct_mutex);
}

/*
 * ospm_resume_display
 *
 * Description: Resume the display hardware restoring state and enabling
 * as necessary.
 */
void ospm_resume_display(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct drm_psb_private *dev_priv = dev->dev_private;
	u32 temp;

	DRM_DEBUG_KMS("\n");

	/* just restore page table ptr */
	PSB_WVDC32(dev_priv->pg->pge_ctl | _PSB_PGETBL_ENABLED,
		   PSB_PGETBL_CTL);
	(void) PSB_RVDC32(PSB_PGETBL_CTL);

	pci_write_config_byte(dev->pdev, 0xF4, dev_priv->saveLBB);

	mutex_lock(&dev->struct_mutex);

	REG_WRITE(DSPCLK_GATE_D, dev_priv->saveDSPCLK_GATE_D);
	REG_WRITE(RAMCLK_GATE_D, dev_priv->saveRAMCLK_GATE_D); 

	/* BIOS does below anyway */
	REG_WRITE(DPIO_CFG, 0);
	REG_WRITE(DPIO_CFG, DPIO_MODE_SELECT_0 | DPIO_CMN_RESET_N);

	temp = REG_READ(DPLL_A);
	if ((temp & DPLL_SYNCLOCK_ENABLE) == 0) {
		REG_WRITE(DPLL_A, temp | DPLL_SYNCLOCK_ENABLE);
		REG_READ(DPLL_A);
	}

	temp = REG_READ(DPLL_B);
	if ((temp & DPLL_SYNCLOCK_ENABLE) == 0) {
		REG_WRITE(DPLL_B, temp | DPLL_SYNCLOCK_ENABLE);
		REG_READ(DPLL_B);
	}
 
	udelay(500);

	REG_WRITE(DSPFW1, dev_priv->saveDSPFW[0]);
	REG_WRITE(DSPFW2, dev_priv->saveDSPFW[1]);
	REG_WRITE(DSPFW3, dev_priv->saveDSPFW[2]);
	REG_WRITE(DSPFW4, dev_priv->saveDSPFW[3]);
	REG_WRITE(DSPFW5, dev_priv->saveDSPFW[4]);
	REG_WRITE(DSPFW6, dev_priv->saveDSPFW[5]);

	REG_WRITE(DSPARB, dev_priv->saveDSPARB);
	REG_WRITE(ADPA, dev_priv->saveADPA);

	REG_WRITE(BLC_PWM_CTL2, dev_priv->saveBLC_PWM_CTL2);
	REG_WRITE(LVDS, dev_priv->saveLVDS);
	REG_WRITE(PFIT_CONTROL, dev_priv->savePFIT_CONTROL);
	REG_WRITE(PFIT_PGM_RATIOS, dev_priv->savePFIT_PGM_RATIOS);
	REG_WRITE(BLC_PWM_CTL, dev_priv->saveBLC_PWM_CTL);
	REG_WRITE(PP_ON_DELAYS, dev_priv->savePP_ON_DELAYS);
	REG_WRITE(PP_OFF_DELAYS, dev_priv->savePP_OFF_DELAYS);
	REG_WRITE(PP_DIVISOR, dev_priv->savePP_DIVISOR);
	REG_WRITE(PP_CONTROL, dev_priv->savePP_CONTROL);

	REG_WRITE(VGACNTRL, dev_priv->saveVGACNTRL);

	REG_WRITE(PSB_INT_ENABLE_R, dev_priv->saveIER);
	REG_WRITE(PSB_INT_MASK_R, dev_priv->saveIMR);

	mutex_unlock(&dev->struct_mutex);

	psb_intel_display_wa(dev);

	drm_mode_config_reset(dev);                                                                                                                                               
	ospm_output_dpms(dev, true);

	/* Resume the modeset for every activated CRTC */                                                                                                                         
	drm_helper_resume_force_mode(dev);
}

/*
 * ospm_suspend_pci
 *
 * Description: Suspend the pci device saving state and disabling
 * as necessary.
 */
static void ospm_suspend_pci(struct pci_dev *pdev)
{

	if (gbSuspended)
		return;

	DRM_DEBUG_KMS("\n");

	pci_save_state(pdev);

	pci_disable_device(pdev);
	pci_set_power_state(pdev, PCI_D3hot);

	gbSuspended = true;
	gbgfxsuspended = true;
}

/*
 * ospm_resume_pci
 *
 * Description: Resume the pci device restoring state and enabling
 * as necessary.
 */
static bool ospm_resume_pci(struct pci_dev *pdev)
{
	int ret = 0;

	if (!gbSuspended)
		return true;

	DRM_DEBUG_KMS("\n");

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);

	ret = pci_enable_device(pdev);

	if (ret != 0)
		DRM_ERROR("ospm_resume_pci: pci_enable_device failed: %d\n", ret);
	else
		gbSuspended = false;

	pci_set_master(pdev);

	return !gbSuspended;
}

/*
 * ospm_power_suspend
 *
 * Description: OSPM is telling our driver to suspend so save state
 * and power down all hardware.
 */
int ospm_power_suspend(struct pci_dev *pdev, pm_message_t state)
{
        int ret = 0;
        int graphics_access_count;
        int videodec_access_count;
    	bool suspend_pci = true; 

	if(gbSuspendInProgress || gbResumeInProgress)
        {
                DRM_ERROR("OSPM_GFX_DPK: %s system BUSY \n", __func__);
                return  -EBUSY;
        }

        mutex_lock(&g_ospm_mutex);

        if (!gbSuspended) {
                graphics_access_count = atomic_read(&g_graphics_access_count);
                videodec_access_count = atomic_read(&g_videodec_access_count);

                if (!ret) {
                        gbSuspendInProgress = true;

			ospm_suspend_display(gpDrmDevice);

			drm_irq_uninstall(gpDrmDevice);
			
			/* Turn off PVR service and power */
			PVRSRVDriverSuspend(gpDrmDevice, state);

			/* FIXME: video driver support for Linux Runtime PM */
                        if (ospm_runtime_pm_msvdx_suspend(gpDrmDevice) != 0) {
				suspend_pci = false;
                        }

			psb_intel_opregion_fini(gpDrmDevice);

                        if (suspend_pci == true) {
				ospm_suspend_pci(pdev);
                        }
                        gbSuspendInProgress = false;
                } else {
                        DRM_DEBUG_KMS("ospm_power_suspend: device busy: graphics %d videodec %d\n",
				  graphics_access_count, videodec_access_count);
                }
        }


        mutex_unlock(&g_ospm_mutex);
        return ret;
}

static void ospm_power_island_wait(u32 reg, u32 mask, u32 target)
{
	int loop_count = 5;
	u32 pwr_sts;

	while (loop_count--) {
		pwr_sts = inl(reg);

		if ((pwr_sts & mask) == target)
			break;
		else
			udelay(10);
	}
}

/*
 * ospm_power_island_up
 *
 * Description: Restore power to the specified island(s) (powergating)
 */
void ospm_power_island_up(int hw_islands)
{
	u32 pwr_cnt = 0;
	u32 pwr_mask = 0;
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) gpDrmDevice->dev_private;

	if (hw_islands & ~(OSPM_GRAPHICS_ISLAND | OSPM_VIDEO_DEC_ISLAND))
		return;

	pwr_cnt = inl(dev_priv->apm_base + PSB_APM_CMD);

	pwr_mask = 0;

	if (hw_islands & OSPM_GRAPHICS_ISLAND) {
		pwr_cnt &= ~PSB_PWRGT_GFX_MASK;
		/* Only the BIT1 is used */
		pwr_cnt |= PSB_PWRGT_GFX_ON;
		pwr_mask = PSB_PWRGT_GFX_MASK;

#ifdef OSPM_STAT
			if (dev_priv->graphics_state == PSB_PWR_STATE_OFF) {
				dev_priv->gfx_off_time += (jiffies - dev_priv->gfx_last_mode_change) * 1000 / HZ;
				dev_priv->gfx_last_mode_change = jiffies;
				dev_priv->graphics_state = PSB_PWR_STATE_ON;
				dev_priv->gfx_on_cnt++;
			}
#endif
		outl(pwr_cnt, dev_priv->apm_base + PSB_APM_CMD);
		ospm_power_island_wait(dev_priv->apm_base + PSB_APM_STS, pwr_mask, PSB_PWRGT_GFX_D0);
	}

	if (hw_islands & OSPM_VIDEO_DEC_ISLAND) {
		pwr_cnt &= ~PSB_PWRGT_VID_DEC_MASK;
		pwr_cnt |= PSB_PWRGT_VID_DEC_ON; 
		pwr_mask = PSB_PWRGT_VID_DEC_MASK;

		outl(pwr_cnt, dev_priv->apm_base + PSB_APM_CMD);
		ospm_power_island_wait(dev_priv->apm_base + PSB_APM_STS, pwr_mask, PSB_PWRGT_VID_DEC_D0);
	}

	g_hw_power_status_mask |= hw_islands;
}

/*
 * ospm_power_resume
 */
int ospm_power_resume(struct pci_dev *pdev)
{
	if(gbSuspendInProgress || gbResumeInProgress)
        {
                DRM_ERROR("OSPM_GFX_DPK: %s hw_island: Suspend || gbResumeInProgress!!!! \n", __func__);
                return 0;
        }

        mutex_lock(&g_ospm_mutex);

	DRM_DEBUG_KMS("\n");

  	gbResumeInProgress = true;

        ospm_resume_pci(pdev);

	psb_intel_clock_gating(gpDrmDevice);

	psb_intel_opregion_setup(gpDrmDevice);

	/* resume PVR power island and service */
	PVRSRVDriverResume(gpDrmDevice);

	ospm_resume_display(gpDrmDevice->pdev);

        mutex_unlock(&g_ospm_mutex);

	drm_irq_install(gpDrmDevice);

	psb_intel_opregion_init(gpDrmDevice);
	
	gbResumeInProgress = false;

	return 0;
}


/*
 * ospm_power_island_down
 *
 * Description: Cut power to the specified island(s) (powergating)
 */
void ospm_power_island_down(int islands)
{
	u32 pwr_cnt = 0;
	u32 pwr_mask = 0;

	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) gpDrmDevice->dev_private;

	if (islands & ~(OSPM_GRAPHICS_ISLAND | OSPM_VIDEO_DEC_ISLAND))
		return;

	pwr_cnt = inl(dev_priv->apm_base + PSB_APM_CMD);

	g_hw_power_status_mask &= ~islands;

	if (islands & OSPM_GRAPHICS_ISLAND) {
		/* Turn off the SGX Power Island */
		pwr_cnt &= ~PSB_PWRGT_GFX_MASK;
		pwr_cnt |= PSB_PWRGT_GFX_OFF;
		pwr_mask = PSB_PWRGT_GFX_MASK;

#ifdef OSPM_STAT
		if (dev_priv->graphics_state == PSB_PWR_STATE_ON) {
			dev_priv->gfx_on_time += (jiffies - dev_priv->gfx_last_mode_change) * 1000 / HZ;
			dev_priv->gfx_last_mode_change = jiffies;
			dev_priv->graphics_state = PSB_PWR_STATE_OFF;
			dev_priv->gfx_off_cnt++;
		}
#endif
		outl(pwr_cnt, dev_priv->apm_base + PSB_APM_CMD);

		ospm_power_island_wait(dev_priv->apm_base + PSB_APM_STS, pwr_mask, PSB_PWRGT_GFX_D3);

	}

	if (islands & OSPM_VIDEO_DEC_ISLAND) {
		/* Turn off the MSVDX power island */
		pwr_cnt &= ~PSB_PWRGT_VID_DEC_MASK;
		pwr_cnt |= PSB_PWRGT_VID_DEC_OFF;
		pwr_mask = PSB_PWRGT_VID_DEC_MASK;

		outl(pwr_cnt, dev_priv->apm_base + PSB_APM_CMD);

		ospm_power_island_wait(dev_priv->apm_base + PSB_APM_STS, pwr_mask, PSB_PWRGT_VID_DEC_D3);
	}
}


/*
 * ospm_power_is_hw_on
 *
 * Description: do an instantaneous check for if the specified islands
 * are on.  Only use this in cases where you know the g_state_change_mutex
 * is already held such as in irq install/uninstall.  Otherwise, use
 * ospm_power_using_hw_begin().
 */
bool ospm_power_is_hw_on(int hw_islands)
{
	return ((g_hw_power_status_mask & hw_islands) == hw_islands) ? true:false;
}

/*
 * ospm_power_using_hw_begin
 *
 * Description: Notify PowerMgmt module that you will be accessing the
 * specified island's hw so don't power it off.  If force_on is true,
 * this will power on the specified island if it is off.
 * Otherwise, this will return false and the caller is expected to not
 * access the hw.  
 * 
 * NOTE *** If this is called from and interrupt handler or other atomic 
 * context, then it will return false if we are in the middle of a
 * power state transition and the caller will be expected to handle that
 * even if force_on is set to true.
 */
bool ospm_power_using_hw_begin(int hw_island, UHBUsage usage)
{
	bool ret = true;
	bool island_is_off = false;
	bool b_atomic = (in_interrupt() || in_atomic());
	bool locked = true;
	struct pci_dev *pdev = gpDrmDevice->pdev;
	bool force_on = usage ? true: false;
	/*quick path, not 100% race safe, but should be enough comapre to current other code in this file */
	if (!force_on) {
		if (hw_island & (OSPM_ALL_ISLANDS & ~g_hw_power_status_mask))
			return false;
		else {
			locked = false;
			goto increase_count;
		}
	}

#ifdef CONFIG_PM_RUNTIME
	/* Anyway, increment pm_runtime_refcount firstly. 
	 * If the return value is false, decrease pm_rumtime_refcount. 
	 */
	pm_runtime_get(&pdev->dev);
#endif
	if (!b_atomic)
		mutex_lock(&g_ospm_mutex);

	island_is_off = hw_island & (OSPM_ALL_ISLANDS & ~g_hw_power_status_mask);

	if (b_atomic && (gbSuspendInProgress || gbResumeInProgress || gbSuspended) && force_on && island_is_off)
		ret = false;

	if (ret && island_is_off && !force_on)
		ret = false;

	if (ret && island_is_off && force_on) {
		gbResumeInProgress = true;

		ret = ospm_resume_pci(pdev);

		if (ret) {
			switch(hw_island)
			{
			case OSPM_GRAPHICS_ISLAND:
				ospm_power_island_up(OSPM_GRAPHICS_ISLAND);
				psb_irq_preinstall_islands(gpDrmDevice, OSPM_GRAPHICS_ISLAND);
				psb_irq_postinstall_islands(gpDrmDevice, OSPM_GRAPHICS_ISLAND);
				break;
			case OSPM_VIDEO_DEC_ISLAND:
				if(!ospm_power_is_hw_on(OSPM_VIDEO_DEC_ISLAND)) {
					/* printk(KERN_ALERT "%s power on video decode\n", __func__); */
					ospm_power_island_up(OSPM_VIDEO_DEC_ISLAND);
					ospm_runtime_pm_msvdx_resume(gpDrmDevice);
					psb_irq_preinstall_islands(gpDrmDevice, OSPM_VIDEO_DEC_ISLAND);
					psb_irq_postinstall_islands(gpDrmDevice, OSPM_VIDEO_DEC_ISLAND);
				}
				else{
					/* printk(KERN_ALERT "%s video decode is already on\n", __func__); */
				}

				break;
			default:
				DRM_ERROR("unknown island !\n");
				break;
			}

		}

		if (!ret)
			DRM_ERROR("ospm_power_using_hw_begin: forcing on %d failed\n", hw_island);

		gbResumeInProgress = false;
	}
increase_count:
	if (ret) {
		switch(hw_island)
		{
		case OSPM_GRAPHICS_ISLAND:
			atomic_inc(&g_graphics_access_count);
			break;
		case OSPM_VIDEO_DEC_ISLAND:
			atomic_inc(&g_videodec_access_count);
			break;
		}
	} 
#ifdef CONFIG_PM_RUNTIME
	else {
		/* decrement pm_runtime_refcount */
		pm_runtime_put(&pdev->dev);
	}
#endif

	if (!b_atomic && locked)
		mutex_unlock(&g_ospm_mutex);

	return ret;
}


/*
 * ospm_power_using_hw_end
 *
 * Description: Notify PowerMgmt module that you are done accessing the
 * specified island's hw so feel free to power it off.  Note that this
 * function doesn't actually power off the islands.
 */
void ospm_power_using_hw_end(int hw_island)
{
	switch(hw_island)
	{
	case OSPM_GRAPHICS_ISLAND:
		atomic_dec(&g_graphics_access_count);
		break;
	case OSPM_VIDEO_DEC_ISLAND:
		atomic_dec(&g_videodec_access_count);
		break;
	}

#ifdef CONFIG_PM_RUNTIME
	/* decrement runtime pm ref count */
	pm_runtime_put(&gpDrmDevice->pdev->dev);
#endif

	WARN_ON(atomic_read(&g_graphics_access_count) < 0);
	WARN_ON(atomic_read(&g_videodec_access_count) < 0);
}

int ospm_runtime_pm_allow(struct drm_device * dev)
{
	struct drm_psb_private * dev_priv = dev->dev_private;

	PSB_DEBUG_ENTRY("%s\n", __FUNCTION__);

	if(dev_priv->rpm_enabled)
		return 0;

	return 0;
}

void ospm_runtime_pm_forbid(struct drm_device * dev)
{
	struct drm_psb_private * dev_priv = dev->dev_private;

	DRM_INFO("%s\n", __FUNCTION__);

	pm_runtime_forbid(&dev->pdev->dev);
	dev_priv->rpm_enabled = 0;
}

int psb_runtime_suspend(struct device *dev)
{
	pm_message_t state;
	int ret = 0;
	state.event = 0;

	DRM_DEBUG_KMS("\n");

        if (atomic_read(&g_graphics_access_count)
		|| atomic_read(&g_videodec_access_count)){
                DRM_ERROR("OSPM_GFX_DPK: GFX: %d VED: %d DSR: %d \n", atomic_read(&g_graphics_access_count),
			  atomic_read(&g_videodec_access_count), 0);
                return -EBUSY;
        }
        else
		ret = ospm_power_suspend(gpDrmDevice->pdev, state);

	return ret;
}

int psb_runtime_resume(struct device *dev)
{
	/* Notify HDMI Audio sub-system about the resume. */
#ifdef CONFIG_SND_INTELMID_HDMI_AUDIO
	struct drm_psb_private* dev_priv = gpDrmDevice->dev_private;

       if(dev_priv->had_pvt_data)
               dev_priv->had_interface->resume(dev_priv->had_pvt_data);
#endif

	/* Nop for GFX */
	return 0;
}

int psb_runtime_idle(struct device *dev)
{
#ifdef CONFIG_SND_INTELMID_HDMI_AUDIO
	struct drm_psb_private* dev_priv = gpDrmDevice->dev_private;
	int hdmi_audio_busy = 0;
	pm_event_t hdmi_audio_event;
#endif

#if 1
	int msvdx_hw_busy = 0;

	msvdx_hw_busy = ospm_runtime_check_msvdx_hw_busy(gpDrmDevice);
#endif

#ifdef CONFIG_SND_INTELMID_HDMI_AUDIO
       if(dev_priv->had_pvt_data){
               hdmi_audio_event.event = 0;
               hdmi_audio_busy = dev_priv->had_interface->suspend(dev_priv->had_pvt_data, hdmi_audio_event);
       }
#endif
	/*printk (KERN_ALERT "lvds:%d,mipi:%d\n", dev_priv->is_lvds_on, dev_priv->is_mipi_on);*/
	if (atomic_read(&g_graphics_access_count)
		|| atomic_read(&g_videodec_access_count)
#ifdef CONFIG_SND_INTELMID_HDMI_AUDIO
		|| hdmi_audio_busy
#endif

#if 1
		|| (msvdx_hw_busy == 1))
#endif
			return 1;
		else
			return 0;
}

