
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

#define PVR_EVENT_SYNC 0x01

/*
 * Every buffer used as a render target has a 'PVRSRV_KERNEL_SYNC_INFO'
 * associated with it. This structure simply contains four counters, number of
 * pending and completed read and write operations. Counters for pending
 * operations are modified (incremented) by the kernel driver while the
 * counters for completed operations are directly updated by the SGX micro
 * kernel. Once both the read counters and the write counters mutually match,
 * one has completed a full frame.
 *
 * Micro kernel issues interrupts whenever TA or 3D phases are completed. These
 * interrupts, however, as such do not tell if any particular frame is
 * completed. It is the responsibility of the kerner driver to determine this.
 * Hence the core interrupt handler calls 'pvr_handle_sync_events' to check if
 * one of the monitored render operations are finished. This is accomplished by
 * examining the before-mentioned counters designated by the pending events.
 */
struct pvr_event_sync {
	struct pvr_event base;
	const struct PVRSRV_KERNEL_SYNC_INFO *sync_info;
	__u64 user_data;
	__u32 tv_sec;
	__u32 tv_usec;
};

/* Event queued up for userspace to read */
struct pvr_pending_event {
	struct pvr_event *event;
	struct list_head link;
	struct PVRSRV_FILE_PRIVATE_DATA *file_priv;
	void (*destroy)(struct pvr_pending_event *event);
};

struct pvr_pending_sync_event {
	struct pvr_pending_event base;
	struct pvr_event_sync event;
};

void pvr_init_events(void);

int pvr_sync_event_req(struct PVRSRV_FILE_PRIVATE_DATA *priv,
			const struct PVRSRV_KERNEL_SYNC_INFO *sync_info,
			u64 user_data);
ssize_t pvr_read(struct file *filp, char __user *buf, size_t count,
		loff_t *off);
unsigned int pvr_poll(struct file *filp, struct poll_table_struct *wait);
void pvr_release_events(struct PVRSRV_FILE_PRIVATE_DATA *priv);

void pvr_handle_sync_events(void);

#endif /* PVR_EVENTS_H */
