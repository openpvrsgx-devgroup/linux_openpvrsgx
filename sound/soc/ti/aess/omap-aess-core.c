/*
 * ALSA SoC driver for OMAP4/5 AESS (Audio Engine Sub-System)
 *
 * Copyright (C) 2010-2013 Texas Instruments
 *
 * Authors: Liam Girdwood <lrg@ti.com>
 *          Misael Lopez Cruz <misael.lopez@ti.com>
 *          Sebastien Guiriec <s-guiriec@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * Contact: Misael Lopez Cruz <misael.lopez@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/pm_opp.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>

#include <sound/soc.h>
#include <sound/soc-topology.h>
#ifdef FIXME	// mechanism does no longer exist since v4.18 and wasn't used anywhere else for long time
#include "../../../arch/arm/mach-omap2/omap-pm.h"
#endif

#include "omap-aess-priv.h"
#include "aess_port.h"
#include "omap-aess.h"
#include "aess_utils.h"
#include "port_mgr.h"
#include "aess_opp.h"
#include "aess_gain.h"

static u64 omap_aess_dmamask = DMA_BIT_MASK(32);

static const char *aess_memory_bank[5] = {
	"dmem",
	"cmem",
	"smem",
	"pmem",
	"mpu"
};

static struct omap_aess *the_aess = NULL;

struct omap_aess *omap_aess_get_handle(void)
{
	struct omap_aess *aess = the_aess;

	if (!aess) {
		pr_err("%s: AESS has not been initialized!\n", __func__);
		return NULL;
	}

	mutex_lock(&aess->mutex);

	aess->nr_users++;

	mutex_unlock(&aess->mutex);

	return aess;
}
EXPORT_SYMBOL(omap_aess_get_handle);

void omap_aess_put_handle(struct omap_aess *aess)
{
	if (!aess)
		return;

	mutex_lock(&aess->mutex);

	if (aess->nr_users == 0)
		goto out;

	aess->nr_users--;
out:
	mutex_unlock(&aess->mutex);
}
EXPORT_SYMBOL(omap_aess_put_handle);

void omap_aess_pm_get(struct omap_aess *aess)
{
	pm_runtime_get_sync(aess->dev);
}
EXPORT_SYMBOL_GPL(omap_aess_pm_get);

void omap_aess_pm_put(struct omap_aess *aess)
{
	pm_runtime_put_sync(aess->dev);
}
EXPORT_SYMBOL_GPL(omap_aess_pm_put);

void omap_aess_pm_shutdown(struct omap_aess *aess)
{
	int ret;

	if (aess->active && omap_aess_check_activity(aess))
		return;

	aess_set_opp_processing(aess, OMAP_AESS_OPP_25);
	aess->opp.level = 25;

	omap_aess_write_event_generator(aess, EVENT_STOP);
	udelay(250);
	if (aess->device_scale) {
		ret = aess->device_scale(aess->dev, aess->dev,
					 aess->opp.freqs[0]);
		if (ret)
			dev_err(aess->dev, "failed to scale to lowest OPP\n");
	}
}
EXPORT_SYMBOL_GPL(omap_aess_pm_shutdown);

void omap_aess_pm_set_mode(struct omap_aess *aess, int mode)
{
	aess->dc_offset.power_mode = mode;
}
EXPORT_SYMBOL(omap_aess_pm_set_mode);

int omap_aess_load_firmware(struct omap_aess *aess, const struct firmware *fw)
{
	int ret;

	if (!aess || !fw)
		return -EINVAL;

	if (aess->fw_loaded)
		return 0;

	if (unlikely(!fw->data)) {
		dev_err(aess->dev, "Loaded firmware is empty\n");
		ret = -EINVAL;
		goto err;
	}

	aess->fw = fw;

	ret = snd_soc_register_component(aess->dev, &omap_aess_platform, omap_aess_dai,
					 ARRAY_SIZE(omap_aess_dai));
	if (ret < 0) {
		dev_err(aess->dev, "failed to register PCM %d\n", ret);
		goto err;
	}

	aess->fw_loaded = true;
	return 0;
err:
	aess->fw_loaded = false;
	return ret;
}
EXPORT_SYMBOL(omap_aess_load_firmware);

static int omap_aess_engine_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct omap_aess *aess;
	int i;

	aess = devm_kzalloc(&pdev->dev, sizeof(struct omap_aess), GFP_KERNEL);
	if (aess == NULL)
		return -ENOMEM;

	for (i = 0; i < OMAP_AESS_IO_RESOURCES; i++) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   aess_memory_bank[i]);
		if (res == NULL) {
			dev_err(&pdev->dev, "no resource: %s\n",
				aess_memory_bank[i]);
			return -ENODEV;
		}
		if (!devm_request_mem_region(&pdev->dev, res->start, 
					     resource_size(res),
					     aess_memory_bank[i]))
			return -EBUSY;

		aess->io_base[i] = devm_ioremap(&pdev->dev, res->start,
					       resource_size(res));
		if (!aess->io_base[i])
			return -ENOMEM;

		if (i == 0)
			aess->dmem_l4 = res->start;
	}

	/* Get needed L3 I/O addresses */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dmem_dma");
	if (res == NULL) {
		dev_err(&pdev->dev, "no L3 resource: dmem\n");
		return -ENODEV;
	}
	aess->dmem_l3 = res->start;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dma");
	if (res == NULL) {
		dev_err(&pdev->dev, "no L3 resource: AESS configuration\n");
		return -ENODEV;
	}
	aess->aess_config_l3 = res->start;

	aess->irq = platform_get_irq(pdev, 0);
	if (aess->irq < 0)
		return aess->irq;

	dev_set_drvdata(&pdev->dev, aess);

#ifdef CONFIG_PM
#ifdef FIXME	// mechanism does no longer exist since v4.18 and wasn't used anywhere else for long time
	aess->get_context_lost_count = omap_pm_get_dev_context_loss_count;
#endif
	aess->device_scale = NULL;
#endif
	aess->dev = &pdev->dev;

	mutex_init(&aess->mutex);
	mutex_init(&aess->opp.mutex);
	mutex_init(&aess->opp.req_mutex);
	INIT_LIST_HEAD(&aess->opp.req);

	spin_lock_init(&aess->lock);
	INIT_LIST_HEAD(&aess->ports);

#ifdef CONFIG_DEBUG_FS
	aess->debugfs_root = debugfs_create_dir("omap-aess", NULL);
	if (!aess->debugfs_root)
		dev_warn(aess->dev, "Failed to create root for debugfs\n");
#endif

	omap_aess_port_mgr_init(aess);

	the_aess = aess;

	get_device(aess->dev);
	aess->dev->dma_mask = &omap_aess_dmamask;
	aess->dev->coherent_dma_mask = omap_aess_dmamask;
	put_device(aess->dev);

	return 0;
}

static int omap_aess_engine_remove(struct platform_device *pdev)
{
	struct omap_aess *aess = dev_get_drvdata(&pdev->dev);

	the_aess = NULL;

	snd_soc_unregister_component(&pdev->dev);
#ifdef CHECKME	// we have no platform device any more - is component sufficient?
// and we register two components - how do we unregister them individually?
	snd_soc_unregister_platform(&pdev->dev);
#endif

	aess->fw_data = NULL;
	aess->fw_config = NULL;
	release_firmware(aess->fw);
	omap_aess_port_mgr_cleanup(aess);

#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(aess->debugfs_root);
#endif

	return 0;
}

static const struct of_device_id omap_aess_of_match[] = {
	{ .compatible = "ti,omap4-aess", },
	{ }
};
MODULE_DEVICE_TABLE(of, omap_aess_of_match);

static struct platform_driver omap_aess_driver = {
	.driver = {
		.name = "aess",
		.owner = THIS_MODULE,
		.of_match_table = omap_aess_of_match,
	},
	.probe = omap_aess_engine_probe,
	.remove = omap_aess_engine_remove,
};

module_platform_driver(omap_aess_driver);

MODULE_ALIAS("platform:omap-aess");
MODULE_DESCRIPTION("ASoC OMAP4 AESS");
MODULE_AUTHOR("Liam Girdwood <lrg@ti.com>");
MODULE_LICENSE("GPL");
