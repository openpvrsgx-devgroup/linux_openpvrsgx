/*

  Copyright(c) 2010-2011 Texas Instruments Incorporated,
  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.
*/

#define uintptr_t host_uintptr_t
#define mode_t host_mode_t
#define dev_t host_dev_t
#define blkcnt_t host_blkcnt_t
typedef int int32_t;

#define _SYS_TYPES_H 1
#include <stdlib.h>
#undef __always_inline
#undef __extern_always_inline
#undef __attribute_const__
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>

#undef __OMAP_AESS_PRIV_H__
#include "socfw.h"

#define CHUNK_SIZE 	4096

struct soc_fw_priv {

	/* opaque vendor data */
	int vendor_fd;
	char *vendor_name;

	/* out file */
	int out_fd;

	int verbose;

	u32 version;

	u32 next_hdr_pos;
};

static void verbose(struct soc_fw_priv *soc_fw, const char *fmt, ...)
{
	int offset = lseek(soc_fw->out_fd, 0, SEEK_CUR);
	va_list va;

	va_start(va, fmt);
	if (soc_fw->verbose) {
		fprintf(stdout, "0x%x/%d -", offset, offset); 
		vfprintf(stdout, fmt, va);
	}
	va_end(va);
}

static int write_header(struct soc_fw_priv *soc_fw, u32 type,
	u32 vendor_type, u32 version, size_t size)
{
	struct snd_soc_tplg_hdr hdr;
	size_t bytes;
	int offset = lseek(soc_fw->out_fd, 0, SEEK_CUR);

	hdr.magic = SND_SOC_TPLG_MAGIC;
	hdr.abi = SND_SOC_TPLG_ABI_VERSION;
	hdr.version = version;
	hdr.type = type;
	hdr.size = sizeof(hdr);
	hdr.vendor_type = vendor_type;
	hdr.payload_size = size;
	hdr.index = 0;
	hdr.count = 1;

	/* make sure the file offset is aligned with the calculated HDR offset */
	if (offset != soc_fw->next_hdr_pos) {
		fprintf(stderr, "error: New header is at offset 0x%x but file"
			 " offset 0x%x is %s by %d bytes\n", soc_fw->next_hdr_pos,
			offset, offset > soc_fw->next_hdr_pos ? "ahead" : "behind",
			 abs(offset - soc_fw->next_hdr_pos));
		exit(-EINVAL);
	}

	fprintf(stdout, "New header type %d size 0x%lx/%ld vendor %d "
		"version %d at offset 0x%x\n", type, (long unsigned int)size, (long int)size, vendor_type,
		version, offset);

	soc_fw->next_hdr_pos += hdr.size + hdr.payload_size;

	bytes = write(soc_fw->out_fd, &hdr, sizeof(hdr));
	if (bytes != sizeof(hdr)) {
		fprintf(stderr, "error: can't write section header %lu\n", (long unsigned int)bytes);
		return bytes;
	}

	return 0;
}

static int tlv_size(const struct snd_kcontrol_new *kcontrol)
{
	struct snd_ctl_tlv *tlv;

	if (!kcontrol->tlv.p)
		return 0;

	tlv = (struct snd_ctl_tlv *)kcontrol->tlv.p;

// length limited to SND_SOC_TPLG_TLV_SIZE?
	return sizeof(struct snd_soc_tplg_ctl_tlv) + tlv->length;
}

static int import_mixer(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrol)
{
	struct snd_soc_tplg_mixer_control mc;
	struct soc_mixer_control *mixer =
		(struct soc_mixer_control *)kcontrol->private_value;
	size_t bytes;

	memset(&mc, 0, sizeof(mc));

	mc.hdr.size = sizeof(mc /* or .hdr? */);
	mc.hdr.type = SND_SOC_TPLG_TYPE_MIXER;
	strncpy(mc.hdr.name, (const char*)kcontrol->name, sizeof(mc.hdr.name));
	mc.hdr.access = kcontrol->access;
#if 0
	mc.hdr.index = kcontrol->index |
		SOC_CONTROL_ID(kcontrol->get, kcontrol->put, 0);
#endif
	// FIXME: where does kcontrol->index go to?
	mc.hdr.ops.get = kcontrol->get;
	mc.hdr.ops.put = kcontrol->put;
	mc.hdr.ops.info = kcontrol->info;
	mc.hdr.tlv.size = tlv_size(kcontrol);
	mc.hdr.tlv.type = 0xaaaa;	// checkme

	mc.size = sizeof(mc /* or .hdr? and depends on num_channels? */);
	mc.min = mixer->min;
	mc.max = mixer->max;
	mc.platform_max = mixer->platform_max;
	mc.invert = mixer->invert;

	mc.num_channels = 2;
	mc.channel[0].size = sizeof(mc.channel[0]);
	mc.channel[0].reg = mixer->reg;
	mc.channel[0].shift = mixer->shift;
	mc.channel[0].id = 0;
	mc.channel[1].size = sizeof(mc.channel[0]);
	mc.channel[1].reg = mixer->rreg;
	mc.channel[1].shift = mixer->rshift;
	mc.channel[1].id = 1;

	verbose(soc_fw," mixer: \"%s\" R1/2 0x%x/0x%x shift L/R %d/%d (g,p,i) %d:%d:%d\n",
		mc.hdr.name, mc.channel[0].reg, mc.channel[1].reg, mc.channel[0].shift, mc.channel[1].shift,
		mc.hdr.ops.get,
		mc.hdr.ops.put,
		mc.hdr.ops.info);

	bytes = write(soc_fw->out_fd, &mc, sizeof(mc));
	if (bytes != sizeof(mc)) {
		fprintf(stderr, "error: can't write mixer %lu\n", (long unsigned int)bytes);
		return bytes;
	}

	return 0;
}

static int import_tlv(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrol)
{
	struct snd_soc_tplg_ctl_tlv *fw_tlv;
	struct snd_ctl_tlv *tlv;
	size_t bytes, size;

	if (!kcontrol->tlv.p)
		return 0;

	tlv = (struct snd_ctl_tlv *)kcontrol->tlv.p;
	verbose(soc_fw, " tlv: type %d length %d\n", tlv->numid, tlv->length);
// if (tlv->length > sizeof(fw_tlv->data) return -EINVAL;
	size = sizeof(*fw_tlv) - sizeof(fw_tlv->data) + tlv->length;
	fw_tlv = calloc(1, size);
	if (!fw_tlv)
		return -ENOMEM;

	fw_tlv->type = tlv->numid;
	fw_tlv->size = tlv->length;
	memcpy(fw_tlv->data, tlv->tlv, tlv->length);

	bytes = write(soc_fw->out_fd, fw_tlv, size);
	free(fw_tlv);
	if (bytes != size) {
		fprintf(stderr, "error: can't write mixer %lu\n", (long unsigned int)bytes);
		return bytes;
	}

	return 0;
}

static void import_enum_control_data(struct soc_fw_priv *soc_fw, int count,
	struct snd_soc_tplg_enum_control *ec, struct soc_enum *menum)
{
	int i;

	if (menum->values) {
		for (i = 0; i < count; i++) {
			ec->values[i] = menum->values[i];
		}
	} else {
		for (i = 0; i < count; i++) {
			strncpy(ec->texts[i], menum->texts[i], sizeof(ec->texts[0]));
		}
	}
}

static int import_enum_control(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrol)
{
	struct snd_soc_tplg_enum_control ec;
	struct soc_enum *menum =
		(struct soc_enum *)kcontrol->private_value;
	size_t bytes;

	memset(&ec, 0, sizeof(ec));

	if (kcontrol->count >= ARRAY_SIZE(ec.texts)) {
		fprintf(stderr, "error: too many enum values %d\n",
			kcontrol->count);
		return -EINVAL;
	}

	ec.hdr.size = sizeof(ec /* or .hdr? */);
	ec.hdr.type = SND_SOC_TPLG_TYPE_ENUM;
	strncpy(ec.hdr.name, (const char*)kcontrol->name, sizeof(ec.hdr.name));
#if 0
	ec.hdr.index = kcontrol->index |
		SOC_CONTROL_ID(kcontrol->get, kcontrol->put, 0);
#endif
	ec.hdr.access = kcontrol->access;
	ec.hdr.ops.get = kcontrol->get;
	ec.hdr.ops.put = kcontrol->put;
	ec.hdr.ops.info = kcontrol->info;
	ec.hdr.tlv.size = tlv_size(kcontrol);
	ec.hdr.tlv.type = 0xaaaa;	// checkme

	ec.size = sizeof(ec);	// checkme
	ec.num_channels = 2;
	ec.channel[0].size = sizeof(ec.channel[0]);
	ec.channel[0].reg = menum->reg;
	ec.channel[0].shift = menum->shift_l;
	ec.channel[0].id = 0;
	ec.channel[1].size = sizeof(ec.channel[0]);
	ec.channel[1].reg = menum->reg;	// same reg?
	ec.channel[1].shift =  menum->shift_r;
	ec.channel[1].id = 1;

	ec.items = menum->max;
	ec.mask = menum->mask;
	ec.count = menum->max;

	import_enum_control_data(soc_fw, ec.items, &ec, menum);

	verbose(soc_fw, " enum: \"%s\" R 0x%x shift L/R %d/%d (g,p,i) %d:%d:%d\n",
		ec.hdr.name, ec.channel[0].reg, ec.channel[0].shift, ec.channel[1].shift,
		ec.hdr.ops.get,
		ec.hdr.ops.put,
		ec.hdr.ops.info);

	bytes = write(soc_fw->out_fd, &ec, sizeof(ec));
	if (bytes != sizeof(ec)) {
		fprintf(stderr, "error: can't write mixer %lu\n", (long unsigned int)bytes);
		return bytes;
	}

	return 0;
}

int socfw_import_controls(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrols, int kcontrol_count)
{
//	struct snd_soc_tplg_bytes_control kc;
	int i /*, mixers = 0, enums = 0 */;
//	size_t bytes, size = sizeof(kc);
	int err;

	if (kcontrol_count == 0)
		return 0;
#if 0
// collection of kcontrols
// does not have an equivalent in soc-topology???

	for (i = 0; i < kcontrol_count; i++) {
		const struct snd_kcontrol_new *kn =
			&kcontrols[i];

		switch (kn->index) {
		case SOC_CONTROL_TYPE_VOLSW:
		case SOC_CONTROL_TYPE_VOLSW_SX:
		case SOC_CONTROL_TYPE_VOLSW_S8:
		case SOC_CONTROL_TYPE_VOLSW_XR_SX:
		case SOC_CONTROL_TYPE_EXT:
			size += sizeof(struct snd_soc_tplg_mixer_control);
			size += tlv_size(kn);
			mixers++;
			break;
		case SOC_CONTROL_TYPE_ENUM:
		case SOC_CONTROL_TYPE_ENUM_EXT:
			size += sizeof(struct snd_soc_tplg_enum_control);
			size += tlv_size(kn);
			enums++;
			break;
		case SOC_CONTROL_TYPE_BYTES:
		case SOC_CONTROL_TYPE_BOOL_EXT:
		case SOC_CONTROL_TYPE_STROBE:
		case SOC_CONTROL_TYPE_RANGE:
		default:
			fprintf(stderr, "error: mixer %s type %d currently not supported\n",
				kn->name, kn->index);
			return -EINVAL;
		}
	}
	verbose(soc_fw, "kcontrols: %d mixers %d enums %d size %lu bytes\n",
		mixers + enums, mixers, enums, size);

	err = write_header(soc_fw, SND_SOC_TPLG_TYPE_MIXER, 0, soc_fw->version, size);
	if (err < 0)
		return err;

	memset(&kc, 0, sizeof(kc));
	kc.count = kcontrol_count;
	
	bytes = write(soc_fw->out_fd, &kc, sizeof(kc));
	if (bytes != sizeof(kc)) {
		fprintf(stderr, "error: can't write mixer %lu\n", (long unsigned int)bytes);
		return bytes;
	}
#endif

	for (i = 0; i < kcontrol_count; i++) {
		const struct snd_kcontrol_new *kn =
			&kcontrols[i];

		switch (kn->index) {
		case SOC_CONTROL_TYPE_VOLSW:
		case SOC_CONTROL_TYPE_VOLSW_SX:
		case SOC_CONTROL_TYPE_VOLSW_S8:
		case SOC_CONTROL_TYPE_VOLSW_XR_SX:
		case SOC_CONTROL_TYPE_EXT:
			err = import_mixer(soc_fw, kn);
			if (err < 0)
				return err;

			err = import_tlv(soc_fw, kn);
			if (err < 0)
				return err;
			break;
		case SOC_CONTROL_TYPE_ENUM:
		case SOC_CONTROL_TYPE_ENUM_EXT:
			err = import_enum_control(soc_fw, kn);
			if (err < 0)
				return err;

			err = import_tlv(soc_fw, kn);
			if (err < 0)
				return err;
			break;
		case SOC_CONTROL_TYPE_BYTES:
		case SOC_CONTROL_TYPE_BOOL_EXT:
		case SOC_CONTROL_TYPE_STROBE:
		case SOC_CONTROL_TYPE_RANGE:
		default:
			fprintf(stderr, "error: mixer %s type %d not supported atm\n",
				kn->name, kn->index);
			return -EINVAL;
		}
	}

	return 0;
}

static void import_coeff_enum_control_data(struct soc_fw_priv *soc_fw,
	const struct snd_soc_fw_coeff *coeff,
	struct snd_soc_tplg_enum_control *ec)
{
	int i;

	for (i = 0; i < coeff->count; i++)
		strncpy(ec->texts[i], coeff->elems[i].description,
			sizeof(ec->texts[0]));
}

static int import_enum_coeff_control(struct soc_fw_priv *soc_fw,
	const struct snd_soc_fw_coeff *coeff)
{
//	struct snd_soc_fw_kcontrol kc;
	struct snd_soc_tplg_enum_control ec;
	struct snd_soc_file_coeff_data cd;
	size_t bytes;
	int err, i;

//	memset(&kc, 0, sizeof(kc));
	memset(&ec, 0, sizeof(ec));

//	kc.count = coeff->count;
	cd.count = coeff->count;
	cd.size = cd.count * coeff->elems[0].size;
	cd.id = coeff->id;

	if (coeff->count >= SND_SOC_TPLG_NUM_TEXTS) {
		fprintf(stderr, "error: too many enum values %d\n",
			coeff->count);
		return -EINVAL;
	}

#if 0 // old

	strncpy(ec.hdr.name, coeff->description, sizeof(ec.hdr.name));
	ec.hdr.index = coeff->index;
	ec.hdr.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
	ec.mask = (coeff->count < 1) - 1;
	ec.items = coeff->count;
	ec.reg = coeff->id;
#endif

// new

	ec.hdr.size = sizeof(ec /* or .hdr? */);
	ec.hdr.type = SND_SOC_TPLG_TYPE_ENUM;
	strncpy(ec.hdr.name, coeff->description, sizeof(ec.hdr.name));
#if 0
	ec.hdr.index = coeff->index;
#endif
	ec.hdr.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
	ec.hdr.ops.get = 0;
	ec.hdr.ops.put = 0;
	ec.hdr.ops.info = 0;
//	ec.hdr.tlv.size = tlv_size(kcontrol);
//	ec.hdr.tlv.type = 0xaaaa;	// checkme

	ec.size = sizeof(ec);	// checkme
	ec.num_channels = 1;
	ec.channel[0].size = sizeof(ec.channel[0]);
	ec.channel[0].reg = coeff->id;
	ec.channel[0].shift = 0;
	ec.channel[0].id = 0;

	ec.items = coeff->count;
	ec.mask = (coeff->count < 1) - 1;
	ec.count = coeff->count;

	import_coeff_enum_control_data(soc_fw, coeff, &ec);

	verbose(soc_fw, " coeff 0x%x enum: \"%s\" R 0x%x shift L/R %d/%d (g,p,i) %d:%d:%d\n",
		cd.id, ec.hdr.name, ec.channel[0].reg, ec.channel[0].shift, ec.channel[1].shift,
		ec.hdr.ops.get,
		ec.hdr.ops.put,
		ec.hdr.ops.info);

#if 0
	bytes = write(soc_fw->out_fd, &kc, sizeof(kc));
	if (bytes != sizeof(kc)) {
		fprintf(stderr, "error: can't write mixer %lu\n", (long unsigned int)bytes);
		return bytes;
	}
#endif

	bytes = write(soc_fw->out_fd, &ec, sizeof(ec));
	if (bytes != sizeof(ec)) {
		fprintf(stderr, "error: can't write mixer %lu\n", (long unsigned int)bytes);
		return bytes;
	}

	err = write_header(soc_fw,  SND_SOC_TPLG_TYPE_PDATA, SND_SOC_TPLG_TYPE_VENDOR_COEFF,
		soc_fw->version, cd.size + sizeof(cd));
	if (err < 0)
		return err;

	bytes = write(soc_fw->out_fd, &cd, sizeof(cd));
	if (bytes != sizeof(cd)) {
		fprintf(stderr, "error: can't write coeff data %lu\n", (long unsigned int)bytes);
		return bytes;
	}

	for (i = 0; i < coeff->count; i++) {
		bytes = write(soc_fw->out_fd, coeff->elems[i].coeffs,
			coeff->elems[i].size);
		if (bytes != coeff->elems[i].size) {
			fprintf(stderr, "error: can't write coeff data %lu\n", (long unsigned int)bytes);
			return bytes;
		}
	}
	return 0;
}

int socfw_import_coeffs(struct soc_fw_priv *soc_fw,
	const struct snd_soc_fw_coeff *coeffs, int coeff_count)
{
	int i, j, enums = 0;
	size_t size = 0;
	int err;

	if (coeff_count == 0)
		return 0;

	for (i = 0; i < coeff_count; i++) {
		size += sizeof(struct snd_soc_tplg_bytes_control)
			+ sizeof(struct snd_soc_tplg_enum_control);
		for (j = 0; j < coeffs[i].count; j++)
			enums++;
	}
	verbose(soc_fw, " coeffs: num coeffs %d enums %d size 0x%lx/%lu bytes\n",
		coeff_count, enums, size, size);

	err = write_header(soc_fw,  SND_SOC_TPLG_TYPE_PDATA, 0,
		soc_fw->version, size);
	if (err < 0)
		return err;

	for (i = 0; i < coeff_count; i++) {
		err = import_enum_coeff_control(soc_fw, &coeffs[i]);
		if (err < 0)
			return err;
	}

	return 0;
}

static int import_dapm_widgets_controls(struct soc_fw_priv *soc_fw, int count,
	const struct snd_kcontrol_new *kn)
{
	int i, err;

	for (i = 0; i < count; i++) {

		switch (kn->index) {
		case SOC_CONTROL_TYPE_VOLSW:
		case SOC_CONTROL_TYPE_VOLSW_SX:
		case SOC_CONTROL_TYPE_VOLSW_S8:
		case SOC_CONTROL_TYPE_VOLSW_XR_SX:
		case SOC_CONTROL_TYPE_EXT:
		case SOC_DAPM_TYPE_VOLSW:
			err = import_mixer(soc_fw, kn);
			if (err < 0)
				return err;

			err = import_tlv(soc_fw, kn);
			if (err < 0)
				return err;

			break;
		case SOC_CONTROL_TYPE_ENUM:
		case SOC_CONTROL_TYPE_ENUM_EXT:
		case SOC_DAPM_TYPE_ENUM_DOUBLE:
		case SOC_DAPM_TYPE_ENUM_EXT:
			err = import_enum_control(soc_fw, kn);
			if (err < 0)
				return err;

			err = import_tlv(soc_fw, kn);
			if (err < 0)
				return err;

			break;
		case SOC_DAPM_TYPE_PIN:
		default:
			fprintf(stderr, "error: widget mixer %s type %d not supported atm\n",
				kn->name, kn->index);
			return -EINVAL;
		}
		kn++;
	}

	return 0;
}

static int socfw_calc_widget_size(struct soc_fw_priv *soc_fw,
	const struct snd_soc_dapm_widget *widgets,
	int widget_count)
{
	int i, j;
	int size = sizeof(*widgets) +
		sizeof(struct snd_soc_tplg_dapm_widget) * widget_count;

	for (i = 0; i < widget_count; i++) {
		const struct snd_kcontrol_new *kn = widgets[i].kcontrol_news;

		for (j = 0; j < widgets[i].num_kcontrols; j++) {

			switch (kn->index) {
			case SOC_CONTROL_TYPE_VOLSW:
			case SOC_CONTROL_TYPE_VOLSW_SX:
			case SOC_CONTROL_TYPE_VOLSW_S8:
			case SOC_CONTROL_TYPE_VOLSW_XR_SX:
			case SOC_CONTROL_TYPE_EXT:
			case SOC_DAPM_TYPE_VOLSW:
				size +=	sizeof(struct snd_soc_tplg_mixer_control);
				break;
			case SOC_CONTROL_TYPE_ENUM:
			case SOC_CONTROL_TYPE_ENUM_EXT:
			case SOC_DAPM_TYPE_ENUM_DOUBLE:
			case SOC_DAPM_TYPE_ENUM_EXT:
				size += sizeof(struct snd_soc_tplg_enum_control);
				break;
			case SOC_DAPM_TYPE_PIN:
			default:
				fprintf(stderr, "error: widget %s mixer %s type %d not supported atm\n",
					widgets[i].name, kn->name, kn->index);
				return -EINVAL;
			}
			kn++;
		}
	}

	return size;
}

int socfw_import_dapm_widgets(struct soc_fw_priv *soc_fw,
	const struct snd_soc_dapm_widget *widgets, int widget_count)
{
	struct snd_soc_tplg_dapm_widget widget;
	struct snd_soc_tplg_hdr delems;
	size_t bytes;
	int i, err, size;

	if (widget_count == 0)
		return 0;

	size = socfw_calc_widget_size(soc_fw, widgets, widget_count);
	if (size == 0)
		return 0;

	verbose(soc_fw, "widgets: widgets %d size %lu bytes\n",
		widget_count, size);

	err = write_header(soc_fw, SND_SOC_TPLG_TYPE_DAPM_WIDGET, 0,
		soc_fw->version, size);
	if (err < 0)
		return err;

	delems.count = widget_count;
	bytes = write(soc_fw->out_fd, &delems, sizeof(delems));
	if (bytes != sizeof(delems)) {
		fprintf(stderr, "error: can't write widget count %d\n",
			(int)bytes);
		return bytes;
	}

	for (i = 0; i < widget_count; i++) {

		memset(&widget, 0, sizeof(widget));

		widget.size = 0;
		widget.id = widgets[i].id;
		strncpy(widget.name, widgets[i].name,
			sizeof(widget.name));

		if (widgets[i].sname)
			strncpy(widget.sname, widgets[i].sname,
				sizeof(widget.sname));

		widget.reg = widgets[i].reg;
		widget.shift = widgets[i].shift;
		widget.mask = widgets[i].mask;
		widget.subseq = 0;
		widget.invert = (widgets[i].on_val == 0);
		widget.ignore_suspend = 0;	/* kept enabled over suspend */
		widget.event_flags = 0;
		widget.event_type = 0;
		widget.num_kcontrols = widgets[i].num_kcontrols;

		verbose(soc_fw, " widget: \"%s\" R 0x%x shift %d\n",
			widget.name, widget.reg, widget.shift);

		bytes = write(soc_fw->out_fd, &widget, sizeof(widget));
		if (bytes != sizeof(widget)) {
			fprintf(stderr, "error: can't write widget %lu\n", (long unsigned int)bytes);
			return bytes;
		}

		err = import_dapm_widgets_controls(soc_fw,
			widgets[i].num_kcontrols,
			widgets[i].kcontrol_news);
		if (err < 0)
			return err;
	}

	return 0;
}

int socfw_import_dapm_graph(struct soc_fw_priv *soc_fw,
	const struct snd_soc_dapm_route *graph, int graph_count)
{
	struct snd_soc_tplg_dapm_graph_elem elem;
	struct snd_soc_tplg_hdr elem_hdr;
	size_t bytes;
	int i, err;

	if (graph_count == 0)
		return 0;

	verbose(soc_fw, "graph: routes %d size %lu bytes\n",
		graph_count, sizeof(elem_hdr) + sizeof(elem) * graph_count);

	err = write_header(soc_fw, SND_SOC_TPLG_TYPE_DAPM_GRAPH, 0, soc_fw->version,
		sizeof(elem_hdr) + sizeof(elem) * graph_count);
	if (err < 0)
		return err;

	elem_hdr.count = graph_count;

	bytes = write(soc_fw->out_fd, &elem_hdr, sizeof(elem_hdr));
	if (bytes != sizeof(elem_hdr)) {
		fprintf(stderr, "error: can't write graph elem header %lu\n", (long unsigned int)bytes);
		return bytes;
	}

	for (i = 0; i <graph_count; i++) {
		strncpy(elem.sink, graph[i].sink, sizeof(elem.sink));
		strncpy(elem.source, graph[i].source, sizeof(elem.source));
		if (graph[i].control)
			strncpy(elem.control, graph[i].control,
				sizeof(elem.control));
		else
			memset(elem.control, 0, sizeof(elem.control));

		verbose(soc_fw, " graph: %s -> %s -> %s\n",
			elem.source, elem.control, elem.sink); 
		bytes = write(soc_fw->out_fd, &elem, sizeof(elem));
		if (bytes != sizeof(elem)) {
			fprintf(stderr, "error: can't write graph elem %lu\n", (long unsigned int)bytes);
			return bytes;
		}
	}

	return 0;
}

int socfw_import_vendor(struct soc_fw_priv *soc_fw, const char *name, int type)
{
	size_t bytes, size;
	char buf[CHUNK_SIZE];
	int i, chunks, rem, err;

	soc_fw->vendor_fd = open(name, O_RDONLY);
	if (soc_fw->vendor_fd < 0) {
		fprintf(stderr, "error: can't open %s %d\n", 
			name, soc_fw->vendor_fd);
		return soc_fw->vendor_fd;
	}


	size = lseek(soc_fw->vendor_fd, 0, SEEK_END);
	if (size <= 0)
		return size;

	verbose(soc_fw, " vendor: file size is %d bytes\n", size);

	err = write_header(soc_fw, type, 0, 0, size);
	if (err < 0)
		return err;

	lseek(soc_fw->vendor_fd, 0, SEEK_SET);

	chunks = size / CHUNK_SIZE;
	rem = size % CHUNK_SIZE;

	for (i = 0; i < chunks; i++) {
		bytes = read(soc_fw->vendor_fd, buf, CHUNK_SIZE);
		if (bytes < 0 || bytes != CHUNK_SIZE) {
			fprintf(stderr, "error: can't read vendor data %lu\n", (long unsigned int)bytes);
			return bytes;
		}

		bytes = write(soc_fw->out_fd, buf, CHUNK_SIZE);
		if (bytes < 0 || bytes != CHUNK_SIZE) {
			fprintf(stderr, "error: can't write vendor data %lu\n", (long unsigned int)bytes);
			return bytes;
		}
	}

	bytes = read(soc_fw->vendor_fd, buf, rem);
	if (bytes < 0 || bytes != rem) {
		fprintf(stderr, "error: can't read vendor data %lu\n", (long unsigned int)bytes);
		return bytes;
	}

	bytes = write(soc_fw->out_fd, buf, rem);
	if (bytes < 0 || bytes != rem) {
		fprintf(stderr, "error: can't write vendor data %lu\n", (long unsigned int)bytes);
		return bytes;
	}

	return 0;
}

int socfw_import_plugin(struct soc_fw_priv *soc_fw, const char *name)
{
	struct snd_soc_fw_plugin *plugin;
	void *plugin_handle;
	int ret;

	fprintf(stdout, "plugin: loading %s\n", name);

	/* load the plugin */
	plugin_handle = dlopen(name, RTLD_LAZY);
	if (!plugin_handle) {
		fprintf(stderr, "error: failed to open %s: %s\n", name,
			dlerror());
		return -EINVAL;
	}

	plugin = dlsym(plugin_handle, "plugin");
	if (!plugin) {
		fprintf(stderr, "error: failed to get symbol. %s\n",
			dlerror());
		dlclose(plugin_handle);
		return -EINVAL;
	}

	/* dump some plugin info */
	fprintf(stdout, "plugin: loaded %s\n", name);
	if (plugin->graph_count > 0)
		fprintf(stdout, " dapm: found graph with %d routes\n",
			plugin->graph_count);
	else
		fprintf(stdout, " dapm: no graph found\n");

	if (plugin->widget_count > 0)
		fprintf(stdout, " dapm: found %d widgets\n",
			plugin->widget_count);
	else
		fprintf(stdout, " dapm: no widgets found\n");

	if (plugin->kcontrol_count > 0)
		fprintf(stdout, " dapm: found %d controls\n",
			plugin->kcontrol_count);
	else
		fprintf(stdout, " dapm: no controls found\n");

	if (plugin->coeff_count > 0)
		fprintf(stdout, " coeff: found %d coefficients\n",
			plugin->coeff_count);
	else
		fprintf(stdout, " coeff: no coefficients found\n");

	soc_fw->version = plugin->version;
	ret = socfw_import_controls(soc_fw, plugin->kcontrols,
		plugin->kcontrol_count);
	if (ret < 0)
		goto out;

	ret = socfw_import_coeffs(soc_fw, plugin->coeffs,
		plugin->coeff_count);
	if (ret < 0)
		goto out;

	ret = socfw_import_dapm_widgets(soc_fw, plugin->widgets,
		plugin->widget_count);
	if (ret < 0)
		goto out;

	ret = socfw_import_dapm_graph(soc_fw, plugin->graph,
		plugin->graph_count);

out:
	dlclose(plugin_handle);
	return ret;
}

struct soc_fw_priv *socfw_new(const char *name, int verbose)
{
	struct soc_fw_priv * soc_fw;
	int fd;

	soc_fw = calloc(1, sizeof(struct soc_fw_priv));
	if (!soc_fw)
		return NULL;

	/* delete any old files */
	unlink(name);

	soc_fw->verbose = verbose;
	fd = open(name, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s err %d\n", name, fd);
		free(soc_fw);
		return NULL;
	}

	soc_fw->out_fd = fd;
	return soc_fw;
}

void socfw_free(struct soc_fw_priv *soc_fw)
{
	close(soc_fw->out_fd);
	free(soc_fw);
}
