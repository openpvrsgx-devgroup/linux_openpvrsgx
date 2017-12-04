#include <linux/version.h>

#include <linux/kernel.h>
#include <mach/hardware.h>
#include <asm/delay.h>
#include <linux/clk.h>
#include <mach/module-owl.h>
#include <linux/clk-provider.h>
#include <../clock-owl.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>

#define ENABLE_COOLING
#ifdef ENABLE_COOLING
#include <linux/cpu_cooling.h>
#endif

enum gpu_clk_mode 
{
	GPU_POWERSAVE = 0,
	GPU_NORMAL,
	GPU_PERFORMANCE_L1,
	GPU_PERFORMANCE_L2,
	GPU_PERFORMANCE_L3,
	GPU_NUMBER_OF_POLICY,
	STRATEGY = 0xff,
};

typedef struct
{
	/* frequency (MHz) */
	unsigned long freq;
	/* vdd requested (uV) */
	unsigned long  vdd;
	/* clock source */
	char pll[20];

} GPUPOLICY;


extern void owl_gpu_clk_enable(void);
extern void owl_gpu_clk_disable(void);

int owl_gpu_clk_notify(struct notifier_block *nb, unsigned long event, void *data);
void owl_gpu_clk_notifier_register(struct notifier_block *notifier);
void owl_gpu_clk_notifier_unregister(struct notifier_block *notifier);

extern int owl_gpu_fun_add_attr(struct kobject *dev_kobj);
int owl_gpu_fun_active(enum gpu_clk_mode mode);

int set_gpu_vdd(struct regulator *gpu_regulator,int voltage);
extern int set_gpu_index(u32 index);
int owlPreClockChange(u32 SGXIndex);
int owlPostClockChange(u32 SGXIndex);
long GPU_Coreclk_Get(void);
int gpu_clock_init(struct device *dev);
int gpu_clock_deinit(void);

int pvr_set_policy(enum gpu_clk_mode mode);

#if 1
extern int gpu_cfg_get_policy(void);
extern int gpu_cfg_get_freq(void);
extern int gpu_cfg_get_vdd(void);
#endif
