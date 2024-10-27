/**************************************************************************
 * Copyright (c) 2011, Intel Corporation.
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
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics.com>
 **************************************************************************/
#ifndef _PSB_SCHEDULE_H_
#define _PSB_SCHEDULE_H_

#include <drm/drmP.h>

struct psb_context;

enum psb_task_type {
	psb_flip_task
};

struct drm_psb_private;

/*struct psb_scheduler_seq {
	uint32_t sequence;
	int reported;
};*/

struct psb_scheduler {
	struct drm_device *dev;
	/*struct psb_scheduler_seq seq[_PSB_ENGINE_TA_FENCE_TYPES];
	struct psb_hw_scene hs[PSB_NUM_HW_SCENES];
	struct mutex task_wq_mutex;*/
	struct mutex msvdx_power_mutex;
	/*spinlock_t lock;
	struct list_head hw_scenes;
	struct list_head ta_queue;
	struct list_head raster_queue;
	struct list_head hp_raster_queue;
	struct list_head task_done_queue;
	struct psb_task *current_task[PSB_SCENE_NUM_ENGINES];
	struct psb_task *feedback_task;
	int ta_state;
	struct psb_hw_scene *pending_hw_scene;
	uint32_t pending_hw_scene_seq;
	struct delayed_work wq*/;
	struct delayed_work msvdx_suspend_wq;
	/*struct psb_scene_pool *pool;
	uint32_t idle_count;
	int idle;
	wait_queue_head_t idle_queue;
	unsigned long ta_end_jiffies;
	unsigned long total_ta_jiffies;
	unsigned long raster_end_jiffies;
	unsigned long total_raster_jiffies;*/
};

/*#define PSB_RF_FIRE_TA       (1 << 0)
#define PSB_RF_OOM           (1 << 1)
#define PSB_RF_OOM_REPLY     (1 << 2)
#define PSB_RF_TERMINATE     (1 << 3)
#define PSB_RF_TA_DONE       (1 << 4)
#define PSB_RF_FIRE_RASTER   (1 << 5)
#define PSB_RF_RASTER_DONE   (1 << 6)
#define PSB_RF_DEALLOC       (1 << 7)
*/

extern int psb_scheduler_init(struct drm_device *dev,
			      struct psb_scheduler *scheduler);

#endif
