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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>

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
	struct snd_soc_fw_hdr hdr;
	size_t bytes;
	int offset = lseek(soc_fw->out_fd, 0, SEEK_CUR);

	hdr.magic = SND_SOC_FW_MAGIC;
	hdr.type = type;
	hdr.vendor_type = vendor_type;
	hdr.version = version;
	hdr.size = size;

	if (offset != soc_fw->next_hdr_pos) {
		fprintf(stderr, "error: New header is at 0x%x but file"
			 " offset is 0x%x delta %d bytes\n", soc_fw->next_hdr_pos,
			offset, offset - soc_fw->next_hdr_pos);
		exit(-EINVAL);
	}

	fprintf(stdout, "New header type %d size 0x%lx/%ld vendor %d "
		"version %d at offset 0x%x\n", type, size, size, vendor_type,
		version, offset);

	soc_fw->next_hdr_pos += hdr.size + sizeof(hdr);

	bytes = write(soc_fw->out_fd, &hdr, sizeof(hdr));
	if (bytes != sizeof(hdr)) {
		fprintf(stderr, "error: can't write section header %lu\n", bytes);
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

	return sizeof(struct snd_soc_fw_ctl_tlv) + tlv->length;
}

static int import_mixer(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrol)
{
	struct snd_soc_fw_mixer_control mc;
	struct soc_mixer_control *mixer =
		(struct soc_mixer_control *)kcontrol->private_value;
	size_t bytes;

	memset(&mc, 0, sizeof(mc));

	strncpy(mc.hdr.name, (const char*)kcontrol->name, SND_SOC_FW_TEXT_SIZE);
	mc.hdr.index = kcontrol->index |
		SOC_CONTROL_ID(kcontrol->get, kcontrol->put, 0);
	mc.hdr.access = kcontrol->access;
	mc.hdr.tlv_size = tlv_size(kcontrol);

	mc.min = mixer->min;
	mc.max = mixer->max;
	mc.platform_max = mixer->platform_max;
	mc.reg = mixer->reg;
	mc.rreg = mixer->rreg;
	mc.shift = mixer->shift;
	mc.rshift = mixer->rshift;
	mc.invert = mixer->invert;

	verbose(soc_fw," mixer: \"%s\" R1/2 0x%x/0x%x shift L/R %d/%d (g,p,i) %d:%d:%d\n",
		mc.hdr.name, mc.reg, mc.rreg, mc.shift, mc.rshift,
		SOC_CONTROL_GET_ID_GET(mc.hdr.index),
		SOC_CONTROL_GET_ID_PUT(mc.hdr.index),
		SOC_CONTROL_GET_ID_INFO(mc.hdr.index));

	bytes = write(soc_fw->out_fd, &mc, sizeof(mc));
	if (bytes != sizeof(mc)) {
		fprintf(stderr, "error: can't write mixer %lu\n", bytes);
		return bytes;
	}

	return 0;
}

static int import_tlv(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrol)
{
	struct snd_soc_fw_ctl_tlv *fw_tlv;
	struct snd_ctl_tlv *tlv;
	size_t bytes, size;

	if (!kcontrol->tlv.p)
		return 0;

	tlv = (struct snd_ctl_tlv *)kcontrol->tlv.p;
	verbose(soc_fw, " tlv: type %d length %d\n", tlv->numid, tlv->length);

	size = sizeof(*fw_tlv) + tlv->length;
	fw_tlv = calloc(1, size);
	if (!fw_tlv)
		return -ENOMEM;

	fw_tlv->numid = tlv->numid;
	fw_tlv->length = tlv->length;
	memcpy(fw_tlv + 1, tlv->tlv, tlv->length);

	bytes = write(soc_fw->out_fd, fw_tlv, size);
	free(fw_tlv);
	if (bytes != size) {
		fprintf(stderr, "error: can't write mixer %lu\n", bytes);
		return bytes;
	}

	return 0;
}

static void import_enum_control_data(struct soc_fw_priv *soc_fw, int count,
	struct snd_soc_fw_enum_control *ec, struct soc_enum *menum)
{
	int i;

	if (menum->values) {
		for (i = 0; i < count; i++) {
			ec->values[i] = menum->values[i];
		}
	} else {
		for (i = 0; i < count; i++) {
			strncpy(ec->texts[i], menum->texts[i], SND_SOC_FW_TEXT_SIZE);
		}
	}
}

static int import_enum_control(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrol)
{
	struct snd_soc_fw_enum_control ec;
	struct soc_enum *menum =
		(struct soc_enum *)kcontrol->private_value;
	size_t bytes;

	memset(&ec, 0, sizeof(ec));

	if (kcontrol->count >= SND_SOC_FW_NUM_TEXTS) {
		fprintf(stderr, "error: too many enum values %d\n",
			kcontrol->count);
		return -EINVAL;
	}

	strncpy(ec.hdr.name, (const char*)kcontrol->name, SND_SOC_FW_TEXT_SIZE);
	ec.hdr.index = kcontrol->index |
		SOC_CONTROL_ID(kcontrol->get, kcontrol->put, 0);
	ec.hdr.access = kcontrol->access;
	ec.hdr.tlv_size = tlv_size(kcontrol);

	ec.mask = menum->mask;
	ec.max = menum->max;
	ec.reg = menum->reg;
	ec.reg2 = menum->reg2;
	ec.shift_l = menum->shift_l;
	ec.shift_r = menum->shift_r;

	verbose(soc_fw, " enum: \"%s\" R1/2 0x%x/0x%x shift L/R %d/%d (g,p,i) %d:%d:%d\n",
		ec.hdr.name, ec.reg, ec.reg2, ec.shift_l, ec.shift_r,
		SOC_CONTROL_GET_ID_GET(ec.hdr.index),
		SOC_CONTROL_GET_ID_PUT(ec.hdr.index),
		SOC_CONTROL_GET_ID_INFO(ec.hdr.index));

	import_enum_control_data(soc_fw, ec.max, &ec, menum);

	bytes = write(soc_fw->out_fd, &ec, sizeof(ec));
	if (bytes != sizeof(ec)) {
		fprintf(stderr, "error: can't write mixer %lu\n", bytes);
		return bytes;
	}

	return 0;
}

int socfw_import_controls(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrols, int kcontrol_count)
{
	struct snd_soc_fw_kcontrol kc;
	int i, mixers = 0, enums = 0;
	size_t bytes, size = sizeof(struct snd_soc_fw_kcontrol);
	int err;

	if (kcontrol_count == 0)
		return 0;

	for (i = 0; i < kcontrol_count; i++) {
		const struct snd_kcontrol_new *kn =
			&kcontrols[i];

		switch (kn->index) {
		case SOC_CONTROL_TYPE_VOLSW:
		case SOC_CONTROL_TYPE_VOLSW_SX:
		case SOC_CONTROL_TYPE_VOLSW_S8:
		case SOC_CONTROL_TYPE_VOLSW_XR_SX:
		//case SOC_CONTROL_TYPE_VOLSW_EXT:
		case SOC_CONTROL_TYPE_EXT:
			size += sizeof(struct snd_soc_fw_mixer_control);
			size += tlv_size(kn);
			mixers++;
			break;
		case SOC_CONTROL_TYPE_ENUM:
		case SOC_CONTROL_TYPE_ENUM_EXT:
		case SOC_CONTROL_TYPE_ENUM_VALUE:
			size += sizeof(struct snd_soc_fw_enum_control);
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

	err = write_header(soc_fw, SND_SOC_FW_MIXER, 0, soc_fw->version, size);
	if (err < 0)
		return err;

	memset(&kc, 0, sizeof(kc));
	kc.count = kcontrol_count;
	
	bytes = write(soc_fw->out_fd, &kc, sizeof(kc));
	if (bytes != sizeof(kc)) {
		fprintf(stderr, "error: can't write mixer %lu\n", bytes);
		return bytes;
	}

	for (i = 0; i < kcontrol_count; i++) {
		const struct snd_kcontrol_new *kn =
			&kcontrols[i];

		switch (kn->index) {
		case SOC_CONTROL_TYPE_VOLSW:
		case SOC_CONTROL_TYPE_VOLSW_SX:
		case SOC_CONTROL_TYPE_VOLSW_S8:
		case SOC_CONTROL_TYPE_VOLSW_XR_SX:
		//case SOC_CONTROL_TYPE_VOLSW_EXT:
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
		case SOC_CONTROL_TYPE_ENUM_VALUE:
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
	struct snd_soc_fw_enum_control *ec)
{
	int i;

	for (i = 0; i < coeff->count; i++)
		strncpy(ec->texts[i], coeff->elems[i].description,
			SND_SOC_FW_TEXT_SIZE);
}

/*
 * Coefficients with mixer.
 *
 * Output:
 *  [kcontrol].[enum].[[hdr].[data]]....
 */
static int import_enum_coeff_control(struct soc_fw_priv *soc_fw,
	const struct snd_soc_fw_coeff *coeff)
{
	struct snd_soc_fw_kcontrol kc;
	struct snd_soc_fw_enum_control ec;
	size_t bytes;
	int err, i;

	memset(&kc, 0, sizeof(kc));
	memset(&ec, 0, sizeof(ec));

	kc.count = coeff->count;

	if (coeff->count >= SND_SOC_FW_NUM_TEXTS) {
		fprintf(stderr, "error: too many enum values %d\n",
			coeff->count);
		return -EINVAL;
	}

	strncpy(ec.hdr.name, coeff->description, SND_SOC_FW_TEXT_SIZE);
	ec.hdr.index = coeff->index;
	ec.hdr.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
	ec.mask = (coeff->count < 1) - 1;
	ec.max = coeff->count;
	ec.reg = coeff->id;

	verbose(soc_fw, " enum: \"%s\" R1/2 0x%x/0x%x shift L/R %d/%d (g,p,i) %d:%d:%d\n",
		ec.hdr.name, ec.reg, ec.reg2, ec.shift_l, ec.shift_r,
		SOC_CONTROL_GET_ID_GET(ec.hdr.index),
		SOC_CONTROL_GET_ID_PUT(ec.hdr.index),
		SOC_CONTROL_GET_ID_INFO(ec.hdr.index));

	import_coeff_enum_control_data(soc_fw, coeff, &ec);

	bytes = write(soc_fw->out_fd, &kc, sizeof(kc));
	if (bytes != sizeof(kc)) {
		fprintf(stderr, "error: can't write mixer %lu\n", bytes);
		return bytes;
	}
	bytes = write(soc_fw->out_fd, &ec, sizeof(ec));
	if (bytes != sizeof(ec)) {
		fprintf(stderr, "error: can't write mixer %lu\n", bytes);
		return bytes;
	}

	for (i = 0; i < coeff->count; i++) {

		err = write_header(soc_fw, SND_SOC_FW_COEFF,
			SND_SOC_FW_VENDOR_COEFF, soc_fw->version,
			coeff->elems[i].size);
		if (err < 0)
			return err;

		bytes = write(soc_fw->out_fd, coeff->elems[i].coeffs,
			coeff->elems[i].size);
		if (bytes != coeff->elems[i].size) {
			fprintf(stderr, "error: can't write mixer %lu\n", bytes);
			return bytes;
		}
	}
	return 0;
}

/*
 * Coefficients with mixer header.
 *
 * Output:
 *  [hdr].<coeff control enum>.<vendor coeff data>
 */
int socfw_import_coeffs(struct soc_fw_priv *soc_fw,
	const struct snd_soc_fw_coeff *coeffs, int coeff_count)
{
	int i, j, enums = 0;
	size_t size = 0;
	int err;

	if (coeff_count == 0)
		return 0;

	for (i = 0; i < coeff_count; i++) {
		size += sizeof(struct snd_soc_fw_kcontrol)
			+ sizeof(struct snd_soc_fw_enum_control);
		for (j = 0; j < coeffs[i].count; j++) {
			//size += coeffs[i].elems[j].size;
			enums++;
		}
	}
	verbose(soc_fw, "coeffs: num coeffs %d enums %d size %lu bytes\n",
		coeff_count, enums, size);

	err = write_header(soc_fw,  SND_SOC_FW_COEFF, 0,
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
		//case SOC_CONTROL_TYPE_VOLSW_EXT:
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
		case SOC_CONTROL_TYPE_ENUM_VALUE:
		case SOC_DAPM_TYPE_ENUM_DOUBLE:
		case SOC_DAPM_TYPE_ENUM_VIRT:
		case SOC_DAPM_TYPE_ENUM_VALUE:
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
	int size = sizeof(struct snd_soc_fw_dapm_elems) +
		sizeof(struct snd_soc_fw_dapm_widget) * widget_count;

	for (i = 0; i < widget_count; i++) {
		const struct snd_kcontrol_new *kn = widgets[i].kcontrol_news;

		for (j = 0; j < widgets[i].num_kcontrols; j++) {

			switch (kn->index) {
			case SOC_CONTROL_TYPE_VOLSW:
			case SOC_CONTROL_TYPE_VOLSW_SX:
			case SOC_CONTROL_TYPE_VOLSW_S8:
			case SOC_CONTROL_TYPE_VOLSW_XR_SX:
		//	case SOC_CONTROL_TYPE_VOLSW_EXT:
			case SOC_CONTROL_TYPE_EXT:
			case SOC_DAPM_TYPE_VOLSW:
				size += /*sizeof(struct snd_soc_fw_kcontrol) +*/
					sizeof(struct snd_soc_fw_mixer_control);
				break;
			case SOC_CONTROL_TYPE_ENUM:
			case SOC_CONTROL_TYPE_ENUM_EXT:
			case SOC_CONTROL_TYPE_ENUM_VALUE:
			case SOC_DAPM_TYPE_ENUM_DOUBLE:
			case SOC_DAPM_TYPE_ENUM_VIRT:
			case SOC_DAPM_TYPE_ENUM_VALUE:
			case SOC_DAPM_TYPE_ENUM_EXT:
				size += /*sizeof(struct snd_soc_fw_kcontrol) +*/
					sizeof(struct snd_soc_fw_enum_control);
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
	struct snd_soc_fw_dapm_widget widget;
	struct snd_soc_fw_dapm_elems delems;
	size_t bytes;
	int i, err, size;

	if (widget_count == 0)
		return 0;

	size = socfw_calc_widget_size(soc_fw, widgets, widget_count);
	if (size == 0)
		return 0;

	verbose(soc_fw, "widgets: widgets %d size %lu bytes\n",
		widget_count, size);

	err = write_header(soc_fw, SND_SOC_FW_DAPM_WIDGET, 0,
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

		strncpy(widget.name, widgets[i].name,
			SND_SOC_FW_TEXT_SIZE);

		if (widgets[i].sname)
			strncpy(widget.sname, widgets[i].sname,
				SND_SOC_FW_TEXT_SIZE);

		widget.id = widgets[i].id;
		widget.reg = widgets[i].reg;
		widget.shift = widgets[i].shift;
		widget.mask = widgets[i].mask;
		widget.invert = widgets[i].invert;
		widget.kcontrol.count = widgets[i].num_kcontrols;

		verbose(soc_fw, " widget: \"%s\" R 0x%x shift %d\n",
			widget.name, widget.reg, widget.shift);

		bytes = write(soc_fw->out_fd, &widget, sizeof(widget));
		if (bytes != sizeof(widget)) {
			fprintf(stderr, "error: can't write widget %lu\n", bytes);
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
	struct snd_soc_fw_dapm_graph_elem elem;
	struct snd_soc_fw_dapm_elems elem_hdr;
	size_t bytes;
	int i, err;

	if (graph_count == 0)
		return 0;

	verbose(soc_fw, "graph: routes %d size %lu bytes\n",
		graph_count, sizeof(elem_hdr) + sizeof(elem) * graph_count);

	err = write_header(soc_fw, SND_SOC_FW_DAPM_GRAPH, 0, soc_fw->version,
		sizeof(elem_hdr) + sizeof(elem) * graph_count);
	if (err < 0)
		return err;

	elem_hdr.count = graph_count;

	bytes = write(soc_fw->out_fd, &elem_hdr, sizeof(elem_hdr));
	if (bytes != sizeof(elem_hdr)) {
		fprintf(stderr, "error: can't write graph elem header %lu\n", bytes);
		return bytes;
	}

	for (i = 0; i <graph_count; i++) {
		strncpy(elem.sink, graph[i].sink, SND_SOC_FW_TEXT_SIZE);
		strncpy(elem.source, graph[i].source, SND_SOC_FW_TEXT_SIZE);
		if (graph[i].control)
			strncpy(elem.control, graph[i].control,
				SND_SOC_FW_TEXT_SIZE);
		else
			memset(elem.control, 0, SND_SOC_FW_TEXT_SIZE);

		verbose(soc_fw, " graph: %s -> %s -> %s\n",
			elem.source, elem.control, elem.sink); 
		bytes = write(soc_fw->out_fd, &elem, sizeof(elem));
		if (bytes != sizeof(elem)) {
			fprintf(stderr, "error: can't write graph elem %lu\n", bytes);
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
			fprintf(stderr, "error: can't read vendor data %lu\n", bytes);
			return bytes;
		}

		bytes = write(soc_fw->out_fd, buf, CHUNK_SIZE);
		if (bytes < 0 || bytes != CHUNK_SIZE) {
			fprintf(stderr, "error: can't write vendor data %lu\n", bytes);
			return bytes;
		}
	}

	bytes = read(soc_fw->vendor_fd, buf, rem);
	if (bytes < 0 || bytes != rem) {
		fprintf(stderr, "error: can't read vendor data %lu\n", bytes);
		return bytes;
	}

	bytes = write(soc_fw->out_fd, buf, rem);
	if (bytes < 0 || bytes != rem) {
		fprintf(stderr, "error: can't write vendor data %lu\n", bytes);
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
