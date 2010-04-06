/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful but, except
 * as otherwise stated in writing, without any warranty; without even the
 * implied warranty of merchantability or fitness for a particular purpose.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK
 *
 ******************************************************************************/

#ifndef __IMG_TYPES_H__
#define __IMG_TYPES_H__

#include <linux/types.h>

#if !defined(IMG_UINT32_MAX)
#define IMG_UINT32_MAX 0xFFFFFFFFUL
#endif

typedef enum tag_img_bool {
	IMG_FALSE = 0,
	IMG_TRUE = 1,
	IMG_FORCE_ALIGN = 0x7FFFFFFF
} IMG_BOOL, *IMG_PBOOL;

struct IMG_CPU_PHYADDR {
	u32 uiAddr;
};

struct IMG_DEV_VIRTADDR {
	u32 uiAddr;
};

struct IMG_DEV_PHYADDR {
	u32 uiAddr;
};

struct IMG_SYS_PHYADDR {
	u32 uiAddr;
};

struct SYSTEM_ADDR {

	u32 ui32PageCount;
	union {
		struct IMG_SYS_PHYADDR sContig;
		struct IMG_SYS_PHYADDR asNonContig[1];
	} u;
};

#include "img_defs.h"

#endif
