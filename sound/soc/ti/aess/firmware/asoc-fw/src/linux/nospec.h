#ifndef _LINUX_NOSPEC_H
#define _LINUX_NOSPEC_H

// surrogate definitions to avoid name clashes

typedef int wait_queue_head_t;
typedef int spinlock_t;
// assume we are on a 32 bit machine like omap4/5
#define array_index_nospec(A, B) (0*A*B)
// #define IS_MODULE(A) (0)
struct snd_pcm_substream { };
struct dentry { };

#endif