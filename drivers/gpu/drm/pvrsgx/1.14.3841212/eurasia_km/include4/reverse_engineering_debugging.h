#ifdef CONFIG_PVR_DRIVER_DEBUG

#include "include/linux/string.h"
#include "include/linux/kernel.h"
void printdbgcomment(char *string, ...){

	va_list aptr;
	char buff[120];

	va_start(aptr,string);
	vsprintf(buff,string, aptr);
	va_end(aptr);

	printk(KERN_ALERT "#PVR_DBG: %s\n", buff);
}

void printwreg(char *dumpregname, u32 reg, u32 data){

	printk(KERN_ALERT "#PVR_DBG: WRW: %s: Reg: 0x%08X  Data: 0x%08X\n", dumpregname, reg, data);
}

void printrreg(char *dumpregname, u32 offset){

	printk(KERN_ALERT "#PVR_DBG: RDW: %s: Reg: 0x%08X  ", dumpregname, offset);
}


#define DEBUG_REG_R printrreg
#define DEBUG_REG_W printwreg

#define RVDBGCOMMENT printdbgcomment
#define DBG_LOG printdbgcomment

#else
#define DEBUG_REG_R(args...)
#define DEBUG_REG_W(args...)
#define RVDBGCOMMENT(args...)
#define DBG_LOG(args...)
#endif
