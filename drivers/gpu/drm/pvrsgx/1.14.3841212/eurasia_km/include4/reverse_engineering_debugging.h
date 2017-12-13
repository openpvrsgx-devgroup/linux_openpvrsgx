#ifdef CONFIG_PVR_DRIVER_DEBUG
#define RV_DBG(x) printk(KERN_ALERT "##PVR_RV_DBG %s", x)
#else
#define RV_DBG(x)
#endif
