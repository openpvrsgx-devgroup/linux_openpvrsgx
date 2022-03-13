#ifndef _LINUX_NOSPEC_H
#define _LINUX_NOSPEC_H
#include <linux/list.h>
typedef int wait_queue_head_t;
typedef int spinlock_t;
typedef int bool;
#define array_index_nospec(A, B) (0*A*B)
#define IS_MODULE(A) (0)
struct snd_pcm_substream { };
struct dentry { };
#include "socfw.h"
#endif
