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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef _PORT_MGR_H_
#define _PORT_MGR_H_

struct snd_pcm_substream;
struct omap_aess;

void omap_aess_port_mgr_init(struct omap_aess *aess);
void omap_aess_port_mgr_cleanup(struct omap_aess *aess);

void omap_aess_port_set_substream(struct omap_aess *aess, int logical_id,
				 struct snd_pcm_substream *substream);
struct snd_pcm_substream *omap_aess_port_get_substream(struct omap_aess *aess,
						      int logical_id);

#endif /* _PORT_MGR_H_ */
