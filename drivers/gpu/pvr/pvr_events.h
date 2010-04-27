
#ifndef PVR_EVENTS_H
#define PVR_EVENTS_H

#include "servicesext.h"
#include "private_data.h"
#include <linux/list.h>
#include <linux/file.h>
#include <linux/poll.h>

/*
 * Header for events written back to userspace on the drm fd. The
 * type defines the type of event, the length specifies the total
 * length of the event (including the header), and user_data is
 * typically a 64 bit value passed with the ioctl that triggered the
 * event.  A read on the drm fd will always only return complete
 * events, that is, if for example the read buffer is 100 bytes, and
 * there are two 64 byte events pending, only one will be returned.
 */
struct pvr_event {
	__u32 type;
	__u32 length;
};

/* Event queued up for userspace to read */
struct pvr_pending_event {
	struct pvr_event *event;
	struct list_head link;
	struct PVRSRV_FILE_PRIVATE_DATA *file_priv;
	void (*destroy)(struct pvr_pending_event *event);
};

void pvr_init_events(void);

ssize_t pvr_read(struct file *filp, char __user *buf, size_t count,
		loff_t *off);
unsigned int pvr_poll(struct file *filp, struct poll_table_struct *wait);
void pvr_release_events(struct PVRSRV_FILE_PRIVATE_DATA *priv);

#endif /* PVR_EVENTS_H */
