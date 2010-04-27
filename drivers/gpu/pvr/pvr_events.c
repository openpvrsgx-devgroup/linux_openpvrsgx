
#include "pvr_events.h"
#include "servicesint.h"

#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

static spinlock_t event_lock;

static bool pvr_dequeue_event(struct PVRSRV_FILE_PRIVATE_DATA *priv,
		size_t total, size_t max, struct pvr_pending_event **out)
{
	struct pvr_pending_event *e;
	unsigned long flags;
	bool ret = false;

	spin_lock_irqsave(&event_lock, flags);

	*out = NULL;
	if (list_empty(&priv->event_list))
		goto err;

	e = list_first_entry(&priv->event_list, struct pvr_pending_event, link);
	if (e->event->length + total > max)
		goto err;

	priv->event_space += e->event->length;
	list_del(&e->link);
	*out = e;
	ret = true;

err:
	spin_unlock_irqrestore(&event_lock, flags);

	return ret;
}

ssize_t pvr_read(struct file *filp, char __user *buf, size_t count, loff_t *off)
{
	struct PVRSRV_FILE_PRIVATE_DATA *priv = filp->private_data;
	struct pvr_pending_event *e;
	size_t total;
	ssize_t ret;

	ret = wait_event_interruptible(priv->event_wait,
					!list_empty(&priv->event_list));

	if (ret < 0)
		return ret;

	total = 0;
	while (pvr_dequeue_event(priv, total, count, &e)) {
		if (copy_to_user(buf + total, e->event, e->event->length)) {
			total = -EFAULT;
			break;
		}

		total += e->event->length;
		e->destroy(e);
	}

	return total;
}

void pvr_release_events(struct PVRSRV_FILE_PRIVATE_DATA *priv)
{
	struct pvr_pending_event *w;
	struct pvr_pending_event *z;
	unsigned long flags;

	spin_lock_irqsave(&event_lock, flags);

	/* Remove unconsumed events */
	list_for_each_entry_safe(w, z, &priv->event_list, link)
		w->destroy(w);

	spin_unlock_irqrestore(&event_lock, flags);
}

void pvr_init_events(void)
{
	spin_lock_init(&event_lock);
}
