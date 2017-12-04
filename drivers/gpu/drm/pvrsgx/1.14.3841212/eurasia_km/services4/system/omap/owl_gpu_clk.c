//2013-7-11 myqi created
//GPU Clock init & control

#include "img_defs.h"
#include "services.h"
#include "kerneldisplay.h"
#include "kernelbuffer.h"
#include "syscommon.h"
#include "pvrmmap.h"
#include "mutils.h"
#include "mm.h"
#include "mmap.h"
#include "mutex.h"
#include "pvr_debug.h"
#include "srvkm.h"
#include "perproc.h"
#include "handle.h"
#include "pvr_bridge_km.h"
#include "proc.h"
#include "private_data.h"
#include "lock.h"
#include "linkage.h"
#include "buffer_manager.h"
#include "syslocal.h"
#include "owl_gpu_clk.h"
#include "power.h"
#include "mach/powergate.h"
#include "owl_gpu_dvfs.h"
#include <linux/proc_fs.h>
#include <linux/of.h>

typedef enum
{
	IC_7059_0_0 = 0,
	IC_7059_0_1 ,
	IC_NUMBER_OF_LEVEL
} ICLEVEL;

typedef struct
{
	ICLEVEL iclevel;/*iclevel*/
	GPUPOLICY gpu_policy[GPU_NUMBER_OF_POLICY];
	
}GPUPOLICY_CONFIG;

GPUPOLICY_CONFIG gpu_policy_list_configs[IC_NUMBER_OF_LEVEL] =
{
	{
		.iclevel=IC_7059_0_0,
		.gpu_policy=
		{
			{240000000,1025000,"DEV_CLK"},
			{300000000,1050000,"DEV_CLK"},
			{400000000,1125000,"DEV_CLK"},
			{480000000,1200000,"DISPLAYPLL"},
			{480000000,1200000,"DISPLAYPLL"},
		}
	},
	{
		.iclevel=IC_7059_0_1,
		.gpu_policy=
		{
			{240000000,1025000,"DEV_CLK"},
			{300000000,1050000,"DEV_CLK"},
			{400000000,1125000,"DEV_CLK"},
			{480000000,1200000,"DISPLAYPLL"},
			{480000000,1200000,"DISPLAYPLL"},
		}
	}
};

struct owl_gpu{
	struct kobject kobj;	
	enum gpu_clk_mode clk_mode;	
	GPUPOLICY_CONFIG *current_gpu_config;
	u32 bactive_clk;
	u32 SGXDeviceIndex;
	u32 ClockChangeCounter;
	unsigned long current_coreclk;
	int normal_policy;
	int bSysInited;
#ifdef ENABLE_COOLING
	int reset_policy;
	int is_cooling;
#endif
};


struct gpu_attribute {
	struct attribute attr;
	ssize_t (*show)(struct owl_gpu *, char *);
	ssize_t	(*store)(struct owl_gpu *, const char *, size_t);
};


#define	DEBUG_PRINTK printk
#define GPU_ATTR(_name, _mode, _show, _store) \
	struct gpu_attribute gpu_attr_##_name = \
	__ATTR(_name, _mode, _show, _store)

/* Define device attribute. */
static ssize_t gpu_devices_show(struct owl_gpu *gpu, char *buf);
static ssize_t gpu_devices_store(struct owl_gpu *gpu,const char *buf, size_t count);
static ssize_t gpu_runtime_show(struct owl_gpu *gpu, char *buf);
static ssize_t gpu_runtime_store(struct owl_gpu *gpu,const char *buf, size_t count);
	
GPU_ATTR(devices,S_IRUGO|S_IWUSR,gpu_devices_show,gpu_devices_store);
GPU_ATTR(runtime,S_IRUGO|S_IWUSR,gpu_runtime_show,gpu_runtime_store);


static struct attribute *gpu_attrs[] = {
	&gpu_attr_devices.attr,
	&gpu_attr_runtime.attr,
	NULL,
};

static ssize_t gpu_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct owl_gpu *gpu;
	struct gpu_attribute *gpu_attr;

	gpu = container_of(kobj, struct owl_gpu, kobj);
	gpu_attr = container_of(attr, struct gpu_attribute, attr);

	if (!gpu_attr->show)
		return -ENOENT;

	return gpu_attr->show(gpu, buf);
}

static ssize_t gpu_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t size)
{
	struct owl_gpu *gpu;
	struct gpu_attribute *gpu_attr;

	gpu = container_of(kobj, struct owl_gpu, kobj);
	gpu_attr = container_of(attr, struct gpu_attribute, attr);

	if (!gpu_attr->store)
		return -ENOENT;

	return gpu_attr->store(gpu, buf, size);
}

struct sysfs_ops gpu_sysops =  
{  
        .show = gpu_attr_show,  
        .store = gpu_attr_store,  
};

struct kobj_type gpu_ktype =  
{  
        .sysfs_ops=&gpu_sysops,  
        .default_attrs=gpu_attrs,  
};

struct owl_gpu owl_gpu;
struct regulator *gpu_regulator=NULL;

int gpu_cfg_get_policy()
{
	return owl_gpu.clk_mode;
}

int gpu_cfg_get_freq()
{
		return owl_gpu.current_gpu_config->gpu_policy[owl_gpu.clk_mode].freq;
}

int gpu_cfg_get_vdd()
{
	return owl_gpu.current_gpu_config->gpu_policy[owl_gpu.clk_mode].vdd;
}


/* add 5 attributes to /sys/devices/b0300000.gpu/pvrsrv */
int owl_gpu_fun_add_attr(struct kobject *dev_kobj)
{
	int err=0;
	err = kobject_init_and_add(&owl_gpu.kobj,&gpu_ktype,dev_kobj,"pvrsrv");
	if (err) 
	{
		printk("failed to create sysfs file\n");
	}
	return err;
}

static ssize_t gpu_devices_show(struct owl_gpu *gpu, char *buf)  
{  
	int ret = 0, policy;
	char *policy_str[GPU_NUMBER_OF_POLICY] = { 
		"powersave", "normal", "performanceL1", "performanceL2", "performanceL3"
	};
	
	policy = gpu->clk_mode;
	ret = sprintf(buf, "%d: %s\n", policy, policy_str[policy]);

	return ret;  
} 

static ssize_t gpu_devices_store(struct owl_gpu *gpu,const char *buf, size_t count) 
{  
	int mode;
	mode = simple_strtoul(buf,NULL,10);
	if(mode == 1005)
	{
		owl_gpu_fun_active(GPU_NORMAL);
	}
	else if(mode==2006)
	{
		owl_gpu_fun_active(GPU_POWERSAVE);
	}
	else if(mode==3007)
	{
		/*Can be configured by dts,such as haier normal_policy is GPU_PERFORMANCE_L1 for performance,
		                                   86v   normal_policy is GPU NORMAL for low power consumption*/
		owl_gpu_fun_active(owl_gpu.normal_policy);
	}
	else if(mode==4008)
	{
		owl_gpu_fun_active(GPU_PERFORMANCE_L3);
	}
	else if(mode==5009)
	{
		owl_gpu_fun_active(GPU_PERFORMANCE_L1);
	}

	return count;
}

static ssize_t gpu_runtime_show(struct owl_gpu *gpu, char *buf)  
{  
	int len;
	unsigned long freq_coreclk,freq_nic_memclk,freq_sysclk,freq_hydclk;
	struct clk *clk;

  clk = clk_get(NULL, "GPU3D_CORECLK");
	freq_coreclk = clk_get_rate(clk);
	freq_coreclk = (freq_coreclk / 1000000);	
	
	clk = clk_get(NULL, "GPU3D_NIC_MEMCLK");
	freq_nic_memclk = clk_get_rate(clk);
	freq_nic_memclk = (freq_nic_memclk / 1000000);	
	
	 clk = clk_get(NULL, "GPU3D_SYSCLK");
	freq_sysclk = clk_get_rate(clk);
	freq_sysclk = (freq_sysclk / 1000000);	
	

	freq_hydclk = clk_get_rate(clk);
	freq_hydclk = (freq_hydclk / 1000000);	
	
	len = sprintf(buf,"core:%ld,mem:%ld,sys:%ld,hyd:%ld\n",freq_coreclk,freq_nic_memclk,freq_sysclk,freq_hydclk);
	return len;  
} 

static ssize_t gpu_runtime_store(struct owl_gpu *gpu,const char *buf, size_t count)  
{  	
	/*   TODO   */
	
	return count;
}

int owl_gpu_clk_notify(struct notifier_block *nb, unsigned long event, void *data)
{
	struct clk_notifier_data *cnd = (struct clk_notifier_data*)data;
	struct clk *gpu_clk = cnd->clk;
	unsigned long new_pll_rate=0, temp_pll_rate=0;
	int new_ratio=0, temp_ratio=0;
	int new_div_idx=0, temp_div_idx=0; 

	if (PRE_RATE_CHANGE == event)
	{
		if(owl_pll_in_change())
		{
	/*		if(owl_gpu.bactive_clk==0)
			{
				DEBUG_PRINTK("NOTIFY_BAD GPU_PERFORMANCE\n");
				return NOTIFY_BAD;
			}
			else*/
			{
				//temp pll div set
				temp_pll_rate = owl_get_putaway_parent_rate(gpu_clk);			
				if(temp_pll_rate<=450*1000000)
					temp_ratio = 2;
				else
					temp_ratio = 3;
				temp_div_idx = owl_getdivider_index(gpu_clk, temp_ratio);

				//new pll div set
				new_pll_rate = owl_getparent_newrate(gpu_clk);
				if(new_pll_rate<=450*1000000)
					new_ratio = 2;
				else
					new_ratio = 3;
				new_div_idx = owl_getdivider_index(gpu_clk, new_ratio);
				
				owl_set_putaway_divsel(gpu_clk, temp_div_idx, new_div_idx);
				owl_update_notify_newrate(gpu_clk, new_pll_rate/new_ratio);
				DEBUG_PRINTK("temp_pll_rate %ld, temp_ratio %d\n",temp_pll_rate,temp_ratio);
				DEBUG_PRINTK("new_pll_rate %ld, new_ratio %d\n",new_pll_rate,new_ratio);

			}
		}
	} 
	else if (POST_RATE_CHANGE == event) 
	{
		//DEBUG_PRINTK("##POST_RATE_CHANGE\n");
	} 
	else if (ABORT_RATE_CHANGE == event) 
	{
		//DEBUG_PRINTK("##ABORT_RATE_CHANGE\n");
	} 
	
	return NOTIFY_OK;
}

void owl_gpu_clk_enable(void)
{
	module_clk_enable(MOD_ID_GPU3D);
#if SUPPORT_GPU_DVFS		
	gpu_dvfs_notify_power_gate_on(&(g_gpu_dvfs_data));
#endif			
	owl_powergate_power_on(OWL_POWERGATE_GPU3D);	
}

void owl_gpu_clk_disable(void)
{
	module_clk_disable(MOD_ID_GPU3D);
#if SUPPORT_GPU_DVFS
	gpu_dvfs_notify_power_gate_off(&(g_gpu_dvfs_data));
#endif
	owl_powergate_power_off(OWL_POWERGATE_GPU3D);
}

void owl_gpu_clk_notifier_register(struct notifier_block *notifier)
{
	struct clk *clk = NULL;
	int ret = -1;
	
	clk = clk_get(NULL, "GPU3D_CORECLK");
	ret = clk_notifier_register(clk, notifier);

	clk = clk_get(NULL, "GPU3D_SYSCLK");
	ret = clk_notifier_register(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_HYDCLK");
	ret = clk_notifier_register(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_NIC_MEMCLK");
	ret = clk_notifier_register(clk, notifier);

}

void owl_gpu_clk_notifier_unregister(struct notifier_block *notifier)
{
	struct clk *clk = NULL;
	int ret = -1;
	
	clk = clk_get(NULL, "GPU3D_CORECLK");
	ret = clk_notifier_unregister(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_SYSCLK");
	ret = clk_notifier_unregister(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_HYDCLK");
	ret = clk_notifier_unregister(clk, notifier);
	
	clk = clk_get(NULL, "GPU3D_NIC_MEMCLK");
	ret = clk_notifier_unregister(clk, notifier);

}

static int owl_set_gpu_clk_rate(unsigned long freq_wanted,const char *clkbuf,const char *pllbuf)
{
	unsigned long freq;
	int ret=-1;
	struct clk *clk= clk_get(NULL, clkbuf);
	struct clk *pll= clk_get(NULL, pllbuf);
	clk_set_parent(clk, pll);
	freq = clk_round_rate(clk, freq_wanted);

   if(freq > 0)
	{
		ret = clk_set_rate(clk, freq);
		if (ret != 0) 
			printk("failed to set gpu clk, %s\n", clkbuf);
	}
	clk_put(clk);
	clk_put(pll);
	return ret;
}

/* TODO:adjust parent pll for some case  */
static unsigned long owl_adjust_pll_freq(unsigned long freq_wanted,const char *pll)
{
	int ret=0;
	struct clk * pll_clk = clk_get(NULL, pll);
	unsigned long pll_freq = clk_get_rate(pll_clk);
	unsigned long pref_freq = 480000000; //480M\A3\ACDISPPLL divider 1

	if(freq_wanted >= pref_freq && 
		(pll_freq < (freq_wanted -12000000) || pll_freq > (freq_wanted +12000000)) )  // temp do it
	{
		unsigned long new_pll_freq;
		
		//DEBUG_PRINTK("freq_wanted: %ld, pll_freq: %ld\n", freq_wanted,pll_freq);
		new_pll_freq = clk_round_rate(pll_clk, freq_wanted);
		if (new_pll_freq!=0) 
		{ 
			ret = clk_set_rate(pll_clk, new_pll_freq);
			if (ret == 0)
			{ 
				DEBUG_PRINTK("set pll freq\n");
			}
			else
			{
				DEBUG_PRINTK("failed to set pll, %s\n", pll);
			}		
		} 
	}
	clk_put(pll_clk);
	return ret;	
} 

static unsigned long get_coreclk_freq(void)
{
	unsigned long freq_coreclk;
	struct clk *clk;

  clk = clk_get(NULL, "GPU3D_CORECLK");
	freq_coreclk = clk_get_rate(clk);
	return freq_coreclk;
}

static unsigned long get_gpu_vdd(void)
{
	unsigned long vdd = 0;
	if(gpu_regulator!=NULL)
	{
		vdd = regulator_get_voltage(gpu_regulator);
	}
	return vdd;
}

static int set_gpu_freq(unsigned long freq_wanted,const char *pll)
{
	int ret = 0, err = 0;
	
	owl_adjust_pll_freq(freq_wanted, pll);
	
	owlPreClockChange(owl_gpu.SGXDeviceIndex);
	
	err = owl_set_gpu_clk_rate(freq_wanted,"GPU3D_CORECLK",pll);
	ret += err;
	err = owl_set_gpu_clk_rate(freq_wanted,"GPU3D_SYSCLK",pll);
	ret += err;
	err = owl_set_gpu_clk_rate(freq_wanted,"GPU3D_NIC_MEMCLK",pll);
	ret += err;
	err = owl_set_gpu_clk_rate(freq_wanted,"GPU3D_HYDCLK",pll);
	ret += err;
	
	owl_gpu.current_coreclk = get_coreclk_freq();  /* get current core clk */
	owlPostClockChange(owl_gpu.SGXDeviceIndex);	
	return ret;

}

int pvr_set_policy(enum gpu_clk_mode mode)
{
	int ret = 0;
	unsigned long freq_wanted;
	int vdd_wanted;
	char pll[20];
	
	//mode = GPU_PERFORMANCE_L1;
	if (mode < 0 || mode >= GPU_NUMBER_OF_POLICY || mode == owl_gpu.clk_mode)
	{
		DEBUG_PRINTK("owl_gpu_fun_active mode not changed %d\n",mode); 
		ret = -1;
		goto exit_out;
	}

	vdd_wanted = owl_gpu.current_gpu_config->gpu_policy[mode].vdd; 
	freq_wanted = owl_gpu.current_gpu_config->gpu_policy[mode].freq;	
	strcpy(pll,owl_gpu.current_gpu_config->gpu_policy[mode].pll); 
	
	owl_gpu.bactive_clk = 1;
	if(mode > owl_gpu.clk_mode) //ascend freq---first ascend vdd then ascend freq
	{
		if(set_gpu_vdd(gpu_regulator,vdd_wanted))
		{
			DEBUG_PRINTK("set_gpu_vdd error!\n");
			ret = -1;
			goto exit_out;
		}
		
		if(set_gpu_freq(freq_wanted,pll))
		{
			DEBUG_PRINTK("set_gpu_freq error!\n");//TODO: exception handle
			ret = -1;
			goto exit_out;
		}
	}
	else if(mode < owl_gpu.clk_mode)//Lower freq---first lower freq then lower vdd
	{
		if(set_gpu_freq(freq_wanted,pll))
		{
			DEBUG_PRINTK("set_gpu_freq error!\n");
			ret = -1;
			goto exit_out;
		}
		
		if(set_gpu_vdd(gpu_regulator,vdd_wanted))
		{
			DEBUG_PRINTK("set_gpu_vdd error!\n");//TODO: exception handle
			ret = -1;
			goto exit_out;
		}
	}
	
	//DEBUG_PRINTK("mode: [%d-->%d], vdd: %ld, freq: %ld !\n",owl_gpu.clk_mode,mode,get_gpu_vdd()/1000, owl_gpu.current_coreclk/1000000);
	DEBUG_PRINTK("mode: [%d-->%d] !\n",owl_gpu.clk_mode,mode);
	owl_gpu.clk_mode = mode;
	exit_out:
	owl_gpu.bactive_clk = 0;
	return ret;
}

int owl_gpu_fun_active(enum gpu_clk_mode mode)
{
	int ret = 0;
	
	#ifdef ENABLE_COOLING
	if(owl_gpu.is_cooling)
	{
		owl_gpu.reset_policy = mode;
		return ret;
	} 
	#endif

  #if SUPPORT_GPU_DVFS
  if(mode !=owl_gpu.normal_policy)
  {
  	gpu_dvfs_deinit(&(g_gpu_dvfs_data));
  }
  #endif
  
	ret = pvr_set_policy(mode);
	
	#if SUPPORT_GPU_DVFS
	if(mode ==owl_gpu.normal_policy)
  {
  	gpu_dvfs_init(&(g_gpu_dvfs_data));
  }
	#endif

	return ret;
}

int set_gpu_vdd(struct regulator *gpu_regulator,int voltage)
{
	int ret;
	if(gpu_regulator!=NULL)
	{
		ret = regulator_set_voltage(gpu_regulator, voltage, INT_MAX);
	}
	else
	{
		printk("gpu regulator NULL!\n");
		ret = -1;
	}
	return ret;
}

int set_gpu_index(u32 index)
{
	owl_gpu.SGXDeviceIndex = index;
	owl_gpu.bSysInited = 1;
	return 0;
}

int owlPreClockChange(u32 SGXIndex)
{
	owl_gpu.ClockChangeCounter++;
	if(owl_gpu.ClockChangeCounter<=1 && owl_gpu.bSysInited)
		PVRSRVDevicePreClockSpeedChange(SGXIndex,1,NULL);
	return 0;
}

int owlPostClockChange(u32 SGXIndex)
{
	if(owl_gpu.ClockChangeCounter<=1 && owl_gpu.bSysInited)
		PVRSRVDevicePostClockSpeedChange(SGXIndex,1,NULL);
	owl_gpu.ClockChangeCounter--;

	return 0;
}

long GPU_Coreclk_Get()
{
	return owl_gpu.current_coreclk;
}
static struct notifier_block owl_gpu_clk_notifier = {
	.notifier_call = owl_gpu_clk_notify,
};

#ifdef ENABLE_COOLING
static int thermal_notifier(struct notifier_block *nb,
								unsigned long event, void *data)
{
	//struct freq_clip_table *notify_table = (struct freq_clip_table *)data;
	if(event == CPUFREQ_COOLING_START)
	{	
		if(owl_gpu.is_cooling == 0){
			owl_gpu.reset_policy = gpu_cfg_get_policy();
			DEBUG_PRINTK("GPU CPUFREQ_COOLING_START,event:%ld,reset_policy:%d\n", event,owl_gpu.reset_policy);
			if(owl_gpu.reset_policy == GPU_PERFORMANCE_L3 
				|| owl_gpu.reset_policy == GPU_PERFORMANCE_L2
				|| owl_gpu.reset_policy == GPU_PERFORMANCE_L1)
			{
				owl_gpu_fun_active(GPU_NORMAL);
			}
			else if(owl_gpu.reset_policy == GPU_NORMAL)
			{
				owl_gpu_fun_active(GPU_POWERSAVE);
			}
			owl_gpu.is_cooling = 1;
		}
	}
	
	if(event == CPUFREQ_COOLING_STOP)
	{
		DEBUG_PRINTK("GPU CPUFREQ_COOLING_STOP event:%ld,reset_policy:%d\n", event,owl_gpu.reset_policy);
		if(owl_gpu.is_cooling == 1){
			owl_gpu.is_cooling = 0;
			owl_gpu_fun_active(owl_gpu.reset_policy);
		}
		
	}
	
	return 0;
}
static struct notifier_block thermal_notifier_block = {
	.notifier_call = thermal_notifier,
};
#endif

static void owl_gpu_regulator_init(struct device *dev)
{
	//get regulator to change vdd_gpu
	if(gpu_regulator==NULL)
	{
		gpu_regulator = regulator_get(dev, "gpuvdd");
		if(IS_ERR(gpu_regulator)) 
		{
			printk("can not get gpuvdd regulator,may be this board not need, or lost in dts!\n");
			gpu_regulator = regulator_get(dev, "dcdc1");
			if(IS_ERR(gpu_regulator))
			{
				  printk("<gpu>err: get regulator failed\n");
					gpu_regulator = NULL;
		  }
		}
	}

}

int gpu_clock_init(struct device *dev)
{
	int ret=0;
	struct device_node *dn;
	u32 normal_value;
	
	int iclevel = 0;   /* need to get from system, default value: IC_7059_0_0 */
	owl_gpu.current_gpu_config = &gpu_policy_list_configs[iclevel]; /* set current gpu config*/
	
	owl_gpu_fun_add_attr(&dev->kobj);	/* add clock attribute configured by user,such as policy*/

	owl_gpu_regulator_init(dev);      /* get gpu regulator */
	
	owl_gpu.normal_policy = GPU_NORMAL;  /* config normal_policy by dts */
	dn = dev->of_node;
	ret =  of_property_read_u32(dn,"normal_value",&normal_value); /* tmp do it */
	if((ret == 0) && (normal_value == 2)) 
		owl_gpu.normal_policy = GPU_PERFORMANCE_L1;
	
	owl_gpu.clk_mode = GPU_POWERSAVE;
	owl_gpu.current_coreclk = owl_gpu.current_gpu_config->gpu_policy[GPU_POWERSAVE].freq;
	owl_gpu.bactive_clk = 0;
	owl_gpu.ClockChangeCounter = 0;
	owl_gpu.bSysInited = 0;
	pvr_set_policy(owl_gpu.normal_policy); /* force set gpu clk rate one time at gpu initial time */
	
	
  /* open to let other module change pll */
	owl_pllsub_set_putaway(CLOCK__GPU3D_CORECLK, CLOCK__DEV_CLK);
	owl_pllsub_set_putaway(CLOCK__GPU3D_NIC_MEMCLK, CLOCK__DEV_CLK);
	owl_pllsub_set_putaway(CLOCK__GPU3D_SYSCLK, CLOCK__DEV_CLK);
	owl_pllsub_set_putaway(CLOCK__GPU3D_HYDCLK, CLOCK__DEV_CLK);
	owl_gpu_clk_notifier_register(&owl_gpu_clk_notifier);
	
	#if SUPPORT_GPU_DVFS
	gpu_dvfs_init(&(g_gpu_dvfs_data));
	#endif
	
	#ifdef ENABLE_COOLING
	cputherm_register_notifier(&thermal_notifier_block, CPUFREQ_COOLING_START);	
	owl_gpu.is_cooling = 0;
	owl_gpu.reset_policy = owl_gpu.normal_policy;
	#endif 
	
	DEBUG_PRINTK("GPU Init success!\n");
	DEBUG_PRINTK("In normal mode %d\n",owl_gpu.normal_policy);
	
	return 0;
}

int gpu_clock_deinit()
{
	owl_gpu_clk_notifier_unregister(&owl_gpu_clk_notifier);
	
	#if SUPPORT_GPU_DVFS
	gpu_dvfs_deinit(&(g_gpu_dvfs_data));
	#endif
	
	#ifdef ENABLE_COOLING
	cputherm_unregister_notifier(&thermal_notifier_block, CPUFREQ_COOLING_START);
	#endif
	return 0;
}
