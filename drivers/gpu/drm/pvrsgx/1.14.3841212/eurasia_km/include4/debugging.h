#ifdef CONFIG_PVR_DRIVER_DEBUG
#error "test"
#define RV_DBG(x) printk(KERN_ALERT x)
#else
#define RV_DBG(x)
#endif
