#include "owl_gpu_dvfs.h"
#include "services_headers.h"
#include "owl_gpu_clk.h"

#include <mach/powergate.h>
#include <linux/cpu_cooling.h>
#include <linux/module.h>
#include <linux/delay.h>

static int start_dvfs(gpu_dvfs_data_t *p_data);
static int stop_dvfs(gpu_dvfs_data_t *p_data);
static int need_video_powersave_mode();
static int do_dvfs_statics(gpu_dvfs_data_t *p_data);
static int dump_dvfs(gpu_dvfs_data_t *p_data);
static int lower_policy(gpu_dvfs_data_t *p_data);
static int raise_policy(gpu_dvfs_data_t *p_data);
static int thermal_cool(gpu_dvfs_data_t *p_data);
static int dvfs_dynamic_strategy(gpu_dvfs_data_t *p_data);
static int reset_dvfs_statics(gpu_dvfs_data_t *p_data);
static void dvfs_work(struct work_struct *work);
static int gpu_dvfs_notify_flip(gpu_dvfs_data_t *p_data);

static ssize_t show_dvfs(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t store_dvfs(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t count);

static struct device_attribute *devattr_dvfs;
static DEVICE_ATTR(dvfs, S_IRUSR|S_IWUSR, show_dvfs, store_dvfs);


gpu_dvfs_data_t g_gpu_dvfs_data = 
{	
	.is_enable = 0,
	.is_cooling = 0,
	.is_dump = 0,
	.is_powergateon = 0,
	.work = 
	{
		.is_init = 0,
		.is_working = 0,
		.work_interval = GPU_DVFS_WORK_INTERVAL,
		.dvfs_workqueue = NULL
	},
	
	.statics = 
	{
		.fps_index = 0,
		.fps_average = 0,
		.fps_video_decode = 0,
		.power_on_times = 0,
		.power_off_times = 0,
		.power_on_ms_count = 0,
		.power_off_ms_count = 0
	},
	
	.thermal = 
	{		
		.reset_policy = 0,
		.vdd_clip_max = -1
	},
	
	.strategy = 
	{
		.powersave_policy = GPUFREQ_POLICY_POWERSAVE,
		.interactive_policy = GPUFREQ_POLICY_INTERACTIVE,
		.performance_policy = GPUFREQ_POLICY_GAME
	}
};	

extern void gpu_dvfs_create_sysfs(struct device *dev)
{
	gpu_dvfs_data_t *p_data = &(g_gpu_dvfs_data); 
	sema_init(&(p_data->sem), 1); 
	if (device_create_file(dev, &dev_attr_dvfs))
	{
		printk(KERN_ERR "PVR_K: %s(%d): " "Failed to create policy device attribute\n",
			   __FUNCTION__, __LINE__);
	}
	else
	{
		devattr_dvfs = &dev_attr_dvfs;
	}
}

extern void gpu_dvfs_remove_sysfs(struct device *dev)
{
	if (devattr_dvfs)
	{
		device_remove_file(dev, devattr_dvfs);
		devattr_dvfs = NULL;
	}
}

extern int gpu_dvfs_init(gpu_dvfs_data_t *p_data)
{
	sema_init(&(p_data->sem), 1); 
	p_data->is_enable = 1;
	p_data->is_cooling = 0;
	p_data->is_dump = 0;
	p_data->is_powergateon = 0;
	reset_dvfs_statics(p_data);
	start_dvfs(p_data);

	return 0;
}

extern int gpu_dvfs_deinit(gpu_dvfs_data_t *p_data)
{

	p_data->is_enable = 0;
	
	stop_dvfs(p_data);
	return 0;
}

extern int gpu_dvfs_notify_power_gate_on(gpu_dvfs_data_t *p_data)
{
	int ret = -1;
	gpu_dvfs_statics_t *p_dvfs_statics = &(p_data->statics);
	
	if(down_interruptible(&(p_data->sem))) {  
		printk(KERN_ERR "%s - %d, down_interruptible error !!!!!!!!!!!!!!\r\n", __FUNCTION__, __LINE__);
        return ret;  
    }
	p_data->is_powergateon = 1;
	do_gettimeofday(&(p_dvfs_statics->power_gate_on_timeval));
	++p_dvfs_statics->power_on_times;
	p_dvfs_statics->power_off_ms_count += ((p_dvfs_statics->power_gate_on_timeval.tv_sec*1000+p_dvfs_statics->power_gate_on_timeval.tv_usec/1000) \
										   - (p_dvfs_statics->power_gate_off_timeval.tv_sec*1000+p_dvfs_statics->power_gate_off_timeval.tv_usec/1000));
	ret = 0;
	
	up(&(p_data->sem));
	return ret;
}

extern int gpu_dvfs_notify_power_gate_off(gpu_dvfs_data_t *p_data)
{
	int ret = -1;
	gpu_dvfs_statics_t *p_dvfs_statics = &(p_data->statics);
	
	if(down_interruptible(&(p_data->sem))) {  
		printk(KERN_ERR "%s - %d, down_interruptible error !!!!!!!!!!!!!!\r\n", __FUNCTION__, __LINE__);
        return ret;  
    }
	p_data->is_powergateon = 0;
	do_gettimeofday(&(p_dvfs_statics->power_gate_off_timeval));
	++p_dvfs_statics->power_off_times;
	p_dvfs_statics->power_on_ms_count += ((p_dvfs_statics->power_gate_off_timeval.tv_sec*1000+p_dvfs_statics->power_gate_off_timeval.tv_usec/1000) \
										  - (p_dvfs_statics->power_gate_on_timeval.tv_sec*1000+p_dvfs_statics->power_gate_on_timeval.tv_usec/1000));
	ret = 0;
	
	up(&(p_data->sem));
	return ret;
}

static ssize_t show_dvfs(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = -1;	
	gpu_dvfs_data_t *p_data = &(g_gpu_dvfs_data);
	
	if(down_interruptible(&(p_data->sem))) {  
		printk(KERN_ERR "%s - %d, down_interruptible error !!!!!!!!!!!!!!\r\n", __FUNCTION__, __LINE__);
        return ret;  
    }
	
	ret = sprintf(buf, "enable=%d, dump=%d, policy %d, %u, %u <= %u.\n", \
				  p_data->is_enable, p_data->is_dump, gpu_cfg_get_policy(), gpu_cfg_get_freq(), gpu_cfg_get_vdd(), g_gpu_dvfs_data.thermal.vdd_clip_max);
	
	up(&(p_data->sem));
	return ret;
}

static ssize_t store_dvfs(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t count)
{			
	int ret = -1;
	gpu_dvfs_data_t *p_data = &(g_gpu_dvfs_data);
	gpu_dvfs_strategy_t* p_dvfs_stragety = &(p_data->strategy);
	
	if(down_interruptible(&(p_data->sem))) {  
		printk(KERN_ERR "%s - %d, down_interruptible error !!!!!!!!!!!!!!\r\n", __FUNCTION__, __LINE__);
        return ret;  
    }
	
	if(!strncmp(buf, "enable=0", strlen("enable=0")))
	{
		p_data->is_enable = 0;
		pvr_set_policy(p_dvfs_stragety->interactive_policy);
	}
	else if(!strncmp(buf, "enable=1", strlen("enable=1")))
	{
		p_data->is_enable = 1;
	}
	else if(!strncmp(buf, "dump=0", strlen("dump=0")))
	{
		p_data->is_dump = 0;
	}
	else if(!strncmp(buf, "dump=1", strlen("dump=1")))
	{
		p_data->is_dump = 1;
	}
	
	up(&(p_data->sem));
	return ret;
}

static int start_dvfs(gpu_dvfs_data_t *p_data)
{
	int ret = -1;
	gpu_dvfs_work_t *p_dvfs_work = &(p_data->work);
	gpu_dvfs_statics_t *p_dvfs_statics = &(p_data->statics);
	
	if(NULL == p_dvfs_work->dvfs_workqueue)
	{
		p_dvfs_work->dvfs_workqueue = create_workqueue("dvfs_workqueue");  
		if (NULL == p_dvfs_work->dvfs_workqueue) {  
			printk(KERN_ERR "%s - %d, no memory for workqueue.\r\n", __FUNCTION__, __LINE__);        
			return ret;     
		}     
		
		INIT_DELAYED_WORK(&(p_dvfs_work->dvfs_delay_work), dvfs_work); 	
				
		do_gettimeofday(&(p_dvfs_statics->power_gate_on_timeval));
		do_gettimeofday(&(p_dvfs_statics->power_gate_off_timeval));
		p_dvfs_work->is_working = 1;
		queue_delayed_work(p_dvfs_work->dvfs_workqueue, &(p_dvfs_work->dvfs_delay_work), 0); 
		ret = 0;
	}
	
    return ret; 
}

static int stop_dvfs(gpu_dvfs_data_t *p_data)
{
	int ret = -1;
	gpu_dvfs_work_t *p_dvfs_work = &(p_data->work);
	
	if(NULL != p_dvfs_work->dvfs_workqueue)
	{
		p_dvfs_work->is_working = 0;
		
		cancel_delayed_work(&(p_dvfs_work->dvfs_delay_work));
		flush_workqueue(p_dvfs_work->dvfs_workqueue);
		destroy_workqueue(p_dvfs_work->dvfs_workqueue);
		p_dvfs_work->dvfs_workqueue = NULL;
		ret = 0;
	} 
	
	return ret;
}
#if 0
static int need_video_powersave_mode()
{
	gpu_dvfs_statics_t *p_dvfs_statics = &(g_gpu_dvfs_data.statics);	
	
	return (p_dvfs_statics->fps_video_decode \
		&& (p_dvfs_statics->power_on_ms_count != (p_dvfs_statics->power_on_ms_count + p_dvfs_statics->power_off_ms_count)));
}
#else
static int need_video_powersave_mode()
{
	return 0;
}
#endif

static int do_dvfs_statics(gpu_dvfs_data_t *p_data)
{
	int index = 0;
	struct timeval cur_timeval;
	gpu_dvfs_statics_t *p_dvfs_statics = &(p_data->statics);
	
	p_dvfs_statics->fps_average = 0;
	for(index=0; index<GPU_DVFS_MAX_STATICS_FPS; ++index)
	{
		p_dvfs_statics->fps_average += p_dvfs_statics->fps[index];
	}
	p_dvfs_statics->fps_average = p_dvfs_statics->fps_average >> 6;  // p_dvfs_statics->average_fps = fps_total / 64
	
	do_gettimeofday(&(cur_timeval));

	if(p_data->is_powergateon)
	{
		p_dvfs_statics->power_on_ms_count += ((cur_timeval.tv_sec*1000+cur_timeval.tv_usec/1000) \
											  - (p_dvfs_statics->power_gate_on_timeval.tv_sec*1000+p_dvfs_statics->power_gate_on_timeval.tv_usec/1000));
	}
	else
	{
		p_dvfs_statics->power_off_ms_count += ((cur_timeval.tv_sec*1000+cur_timeval.tv_usec/1000) \
											   - (p_dvfs_statics->power_gate_off_timeval.tv_sec*1000+p_dvfs_statics->power_gate_off_timeval.tv_usec/1000));
	}
	do_gettimeofday(&(p_dvfs_statics->power_gate_on_timeval));
	do_gettimeofday(&(p_dvfs_statics->power_gate_off_timeval));	
	
	return 0;
}

static int dump_dvfs(gpu_dvfs_data_t *p_data)
{
	gpu_dvfs_statics_t *p_dvfs_statics = &(p_data->statics);
	
	if(NULL != p_dvfs_statics)
	{		
		// printk(KERN_ERR "%s - %d ---------------.\r\n", __FUNCTION__, __LINE__);
		printk(KERN_ERR "gpu_dvfs: %2d, %2d, %2d, %2d, %4d / %4d, %4d, %1d.\r\n", \
			   p_dvfs_statics->fps_index, \
			   p_dvfs_statics->fps[p_dvfs_statics->fps_index], \
			   p_dvfs_statics->fps_average, \
			   p_dvfs_statics->fps_video_decode, \
			   p_dvfs_statics->power_on_ms_count, p_dvfs_statics->power_on_ms_count + p_dvfs_statics->power_off_ms_count, \
			   p_dvfs_statics->power_off_times, \
			   gpu_cfg_get_policy());			
	}
	
	return 0;
}

static int lower_policy(gpu_dvfs_data_t *p_data)
{
	int ret = -1;
	gpu_dvfs_statics_t* p_dvfs_statics = &(p_data->statics);
	gpu_dvfs_strategy_t* p_dvfs_stragety = &(p_data->strategy);
	
	int is_idle_per_fps = p_dvfs_statics->fps[p_dvfs_statics->fps_index] * GPU_DVFS_STRATEGY_TRIGGER_POLICY_DOWN_FPS_FACTOR <= \
		p_dvfs_statics->power_off_times * GPU_DVFS_STRATEGY_TRIGGER_POLICY_DOWN_POWER_GATE_OFF_FACTOR;
	int is_bottleneck = ((p_dvfs_statics->power_on_ms_count - (SYS_SGX_ACTIVE_POWER_LATENCY_MS * p_dvfs_statics->power_off_times))>= \
						 GPU_DVFS_STRATEGY_TRIGGER_POLICY_UP_MS_TH) \
		&& (p_dvfs_statics->fps[p_dvfs_statics->fps_index] * GPU_DVFS_STRATEGY_TRIGGER_POLICY_UP_FPS_FACTOR >= \
			p_dvfs_statics->power_off_times * GPU_DVFS_STRATEGY_TRIGGER_POLICY_UP_POWER_GATE_OFF_FACTOR);
	int is_poor_fps = p_dvfs_statics->fps[p_dvfs_statics->fps_index] <= GPU_DVFS_STRATEGY_TRIGGER_PERFORMANCE_FPS_TH;
	int is_need_performance_policy = is_bottleneck && is_poor_fps;
		
	if((p_dvfs_stragety->performance_policy==gpu_cfg_get_policy()) \
	   && (is_idle_per_fps||!is_need_performance_policy))
	{		
		pvr_set_policy(p_dvfs_stragety->interactive_policy);
		ret = 0;
	}
	else if((p_dvfs_stragety->interactive_policy==gpu_cfg_get_policy()) \
			&& (is_idle_per_fps || need_video_powersave_mode()))
	{		
		pvr_set_policy(p_dvfs_stragety->powersave_policy);
		ret = 0;		
	}
	
	return ret;
}

static int raise_policy(gpu_dvfs_data_t *p_data)
{
	int ret = -1;
	gpu_dvfs_statics_t* p_dvfs_statics = &(p_data->statics);
	gpu_dvfs_strategy_t* p_dvfs_stragety = &(p_data->strategy);
	
	int is_bottleneck = ((p_dvfs_statics->power_on_ms_count - (SYS_SGX_ACTIVE_POWER_LATENCY_MS * p_dvfs_statics->power_off_times))>= \
						 GPU_DVFS_STRATEGY_TRIGGER_POLICY_UP_MS_TH) \
		&& (p_dvfs_statics->fps[p_dvfs_statics->fps_index] * GPU_DVFS_STRATEGY_TRIGGER_POLICY_UP_FPS_FACTOR >= \
			p_dvfs_statics->power_off_times * GPU_DVFS_STRATEGY_TRIGGER_POLICY_UP_POWER_GATE_OFF_FACTOR);
	int is_poor_fps = p_dvfs_statics->fps[p_dvfs_statics->fps_index] <= GPU_DVFS_STRATEGY_TRIGGER_PERFORMANCE_FPS_TH;
	int is_need_performance_policy = is_bottleneck && is_poor_fps;
	
	if((p_dvfs_stragety->powersave_policy==gpu_cfg_get_policy())
	   && !need_video_powersave_mode() \
	   && is_bottleneck)
	{		
		pvr_set_policy(p_dvfs_stragety->interactive_policy);
		ret = 0;
	}
	else if((p_dvfs_stragety->interactive_policy == gpu_cfg_get_policy()) \
			&& is_need_performance_policy)
	{	
		pvr_set_policy(p_dvfs_stragety->performance_policy);
		ret = 0;	
	}
	
	return ret;
}

static int thermal_cool(gpu_dvfs_data_t *p_data)
{	
	int ret = -1;
	gpu_dvfs_thermal_t* p_dvfs_thermal = &((p_data)->thermal);
	if(1 == p_data->is_cooling)
	{
		pvr_limit_vdd_dvfs(p_dvfs_thermal->vdd_clip_max);
		ret = 0;
	}
	else if (2 == p_data->is_cooling)
	{
		pvr_set_policy(p_dvfs_thermal->reset_policy);
		p_data->is_cooling = 0;
		ret = 0;
	}
	
	return ret;
}

static int dvfs_dynamic_strategy(gpu_dvfs_data_t *p_data)
{	
	if(!lower_policy(p_data))
	{
		// printk(KERN_ERR "lower_policy -> %d, %u, %u.\r\n", gpu_cfg_get_policy(), gpu_cfg_get_freq(), gpu_cfg_get_vdd());
	}
	else if(!raise_policy(p_data))
	{
		// printk(KERN_ERR "raise_policy -> %d, %u, %u.\r\n", gpu_cfg_get_policy(), gpu_cfg_get_freq(), gpu_cfg_get_vdd());
	}
	
	return 0;
}

static int reset_dvfs_statics(gpu_dvfs_data_t *p_data)
{
	gpu_dvfs_statics_t* p_dvfs_statics = &((p_data)->statics);
	
	p_dvfs_statics->fps_index = (p_dvfs_statics->fps_index+1)%GPU_DVFS_MAX_STATICS_FPS;
	p_dvfs_statics->fps[p_dvfs_statics->fps_index] = 0;	
	p_dvfs_statics->fps_video_decode = 0;
	p_dvfs_statics->power_on_times = 0;
	p_dvfs_statics->power_off_times = 0;
	p_dvfs_statics->power_on_ms_count = 0;
	p_dvfs_statics->power_off_ms_count = 0;				
	
	return 0;
}

static void dvfs_work(struct work_struct *work)  
{  	
	g_gpu_dvfs_data.work.is_init = 0;
	while(g_gpu_dvfs_data.work.is_working)
	{
		do_dvfs_statics(&g_gpu_dvfs_data);		
		
		if(g_gpu_dvfs_data.work.is_init)
		{
			if(g_gpu_dvfs_data.is_dump)
			{
				dump_dvfs(&g_gpu_dvfs_data);	
			}
			
			if(g_gpu_dvfs_data.is_cooling)
			{
				thermal_cool(&g_gpu_dvfs_data);				
			}
			else if(g_gpu_dvfs_data.is_enable)
			{				
				dvfs_dynamic_strategy(&g_gpu_dvfs_data);
			}	
		}					
		g_gpu_dvfs_data.work.is_init = 1;
		
		// reset statics data
		reset_dvfs_statics((&(g_gpu_dvfs_data)));
		
		msleep(g_gpu_dvfs_data.work.work_interval);
		// printk(KERN_ERR "policy %d, %u, %u <= %u.\r\n", gpu_cfg_get_policy(), gpu_cfg_get_freq(), gpu_cfg_get_vdd(), g_gpu_dvfs_data.thermal.vdd_clip_max);
	}	
}  

static int gpu_dvfs_notify_flip(gpu_dvfs_data_t *p_data)
{
	#if SUPPORT_GPU_DVFS
	gpu_dvfs_statics_t *p_dvfs_statics = &(p_data->statics);
	
	++p_dvfs_statics->fps[p_dvfs_statics->fps_index];
	#endif
	return 0;
}
	
extern void gpu_dvfs_notify_vde_decode_kick(void)
{	
	++g_gpu_dvfs_data.statics.fps_video_decode;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_notify_vde_decode_kick);

extern void NotifySystemFlip()
{
	gpu_dvfs_notify_flip(&(g_gpu_dvfs_data));	
}


EXPORT_SYMBOL(NotifySystemFlip);

