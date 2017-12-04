#if !defined(__OWL_GPU_DVFS_H__)
#define __OWL_GPU_DVFS_H__

#include <linux/time.h>
#include <linux/semaphore.h>   
#include <linux/device.h>
#include <linux/workqueue.h>  

#define SUPPORT_GPU_DVFS 0

#define GPU_DVFS_MAX_STATICS_FPS 64
#define GPU_DVFS_WORK_INTERVAL 1000

#define GPU_DVFS_STRATEGY_TRIGGER_POLICY_DOWN_FPS_FACTOR 1
#define GPU_DVFS_STRATEGY_TRIGGER_POLICY_DOWN_POWER_GATE_OFF_FACTOR 2

#define GPU_DVFS_STRATEGY_TRIGGER_POLICY_UP_FPS_FACTOR 1
#define GPU_DVFS_STRATEGY_TRIGGER_POLICY_UP_POWER_GATE_OFF_FACTOR 3
#define GPU_DVFS_STRATEGY_TRIGGER_POLICY_UP_MS_TH 980

#define GPU_DVFS_STRATEGY_TRIGGER_PERFORMANCE_FPS_TH 60


#define GPUFREQ_POLICY_POWERSAVE     GPU_POWERSAVE
#define GPUFREQ_POLICY_INTERACTIVE   GPU_NORMAL
#define GPUFREQ_POLICY_GAME          GPU_PERFORMANCE_L1


//#define cputherm_register_notifier(a,b) 
//#define cputherm_unregister_notifier(a,b)
#define pvr_limit_vdd_dvfs(a) 

#define CPUFREQ_COOLING_START 0
#define CPUFREQ_COOLING_STOP  1

typedef struct _gpu_dvfs_work_
{
	unsigned int is_init;
	unsigned int is_working;
	unsigned int work_interval;
	struct workqueue_struct *dvfs_workqueue;  
	struct delayed_work dvfs_delay_work; 
}gpu_dvfs_work_t;
	
typedef struct _gpu_dvfs_statics_
{		
	unsigned int fps_index;
	unsigned int fps[GPU_DVFS_MAX_STATICS_FPS];	
	unsigned int fps_average;
	unsigned int fps_video_decode;
	unsigned int power_on_times;
	unsigned int power_off_times;
	unsigned int power_on_ms_count;
	unsigned int power_off_ms_count;
	struct timeval power_gate_on_timeval;
	struct timeval power_gate_off_timeval;
}gpu_dvfs_statics_t;

typedef struct _gpu_dvfs_thermal_
{	
	int reset_policy;
	unsigned int vdd_clip_max;
}gpu_dvfs_thermal_t;
	
typedef struct _gpu_dvfs_strategy_
{
	int powersave_policy;
	int interactive_policy;
	int performance_policy;
}gpu_dvfs_strategy_t;

typedef struct _gpu_dvfs_data_
{	
	struct semaphore sem;	
	int is_enable;
	int is_cooling;
	int is_dump;
	int is_enable_performance;
	int is_powergateon;
	gpu_dvfs_work_t work;
	gpu_dvfs_statics_t statics;
	gpu_dvfs_thermal_t thermal;
	gpu_dvfs_strategy_t strategy;
}gpu_dvfs_data_t;

extern void gpu_dvfs_create_sysfs(struct device *dev);
extern void gpu_dvfs_remove_sysfs(struct device *dev);

extern int gpu_dvfs_init(gpu_dvfs_data_t *p_data);
extern int gpu_dvfs_deinit(gpu_dvfs_data_t *p_data);
extern int gpu_dvfs_notify_power_gate_on(gpu_dvfs_data_t *p_data);
extern int gpu_dvfs_notify_power_gate_off(gpu_dvfs_data_t *p_data);

extern gpu_dvfs_data_t g_gpu_dvfs_data;

#endif
